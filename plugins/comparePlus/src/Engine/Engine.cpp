/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "diff.h"

#include <cassert>
#include <climits>
#include <cstdint>
#include <exception>
#include <utility>
#include <vector>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <functional>

#include <windows.h>

#include "Tools.h"
#include "Engine.h"
#include "ProgressDlg.h"


#ifdef MULTITHREAD

#include <atomic>

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#else
#include <thread>
#endif // __MINGW32__ ...

#else // MULTITHREAD not defined

#pragma message("Multithread change detection disabled.")

#endif // MULTITHREAD


namespace {

static constexpr uint64_t cHashSeed = 0x84222325;


enum class charType
{
	SPACECHAR,
	ALPHANUMCHAR,
	OTHERCHAR
};


struct Line
{
	Line(intptr_t l = 0, uint64_t h = cHashSeed) : line(l), hash(h) {}

	intptr_t line;

	uint64_t hash;

	inline bool operator==(const Line& rhs) const
	{
		return (hash == rhs.hash);
	}

	inline bool operator!=(const Line& rhs) const
	{
		return (hash != rhs.hash);
	}

	inline bool operator==(uint64_t rhs) const
	{
		return (hash == rhs);
	}

	inline bool operator!=(uint64_t rhs) const
	{
		return (hash != rhs);
	}
};


struct Word
{
	Word(intptr_t p, intptr_t l, uint64_t h = cHashSeed) : pos(p), len(l), hash(h) {}

	intptr_t pos;
	intptr_t len;

	uint64_t hash;

	inline bool operator==(const Word& rhs) const
	{
		return (hash == rhs.hash);
	}

	inline bool operator!=(const Word& rhs) const
	{
		return (hash != rhs.hash);
	}

	inline bool operator==(uint64_t rhs) const
	{
		return (hash == rhs);
	}

	inline bool operator!=(uint64_t rhs) const
	{
		return (hash != rhs);
	}
};


struct Char
{
	Char(wchar_t c, intptr_t p) : ch(c), pos(p) {}

	wchar_t ch;
	intptr_t pos;

	inline bool operator==(const Char& rhs) const
	{
		return (ch == rhs.ch);
	}

	inline bool operator!=(const Char& rhs) const
	{
		return (ch != rhs.ch);
	}

	inline bool operator==(wchar_t rhs) const
	{
		return (ch == rhs);
	}

	inline bool operator!=(wchar_t rhs) const
	{
		return (ch != rhs);
	}
};


struct DocCmpInfo
{
	int			view;
	section_t	section;

	int			blockDiffMask;

	std::vector<Line>				lines;
	std::unordered_set<intptr_t>	nonUniqueLines;
};


struct diffLine
{
	diffLine(intptr_t lineNum) : line(lineNum) {}

	intptr_t line;
	std::vector<line_section_t> changes;
};


struct blockDiffInfo
{
	const diff_info<blockDiffInfo>*	matchBlock {nullptr};

	std::vector<diffLine>	changedLines;
	std::vector<section_t>	moves;

	inline intptr_t movedCount() const
	{
		intptr_t count = 0;

		for (const auto& move: moves)
			count += move.len;

		return count;
	}

	inline intptr_t movedSection(intptr_t line) const
	{
		for (const auto& move: moves)
		{
			if (line >= move.off && line < move.off + move.len)
				return move.len;
		}

		return 0;
	}

	inline bool getNextUnmoved(intptr_t& line) const
	{
		for (const auto& move: moves)
		{
			if (line >= move.off && line < move.off + move.len)
			{
				line = move.off + move.len;
				return true;
			}
		}

		return false;
	}
};


using diffInfo = diff_info<blockDiffInfo>;


struct CompareInfo
{
	// Input data
	DocCmpInfo				doc1;
	DocCmpInfo				doc2;

	// Output data - filled by the compare engine
	diff_results<blockDiffInfo>	blockDiffs;
};


struct MatchInfo
{
	intptr_t	lookupOff;
	diffInfo*	matchDiff;
	intptr_t	matchOff;
	intptr_t	matchLen;
};


struct LinesConv
{
	float conv;

	intptr_t line1;
	intptr_t line2;

	LinesConv() : line1(-1), line2(-1)
	{}

	LinesConv(float c, intptr_t l1, intptr_t l2) : conv(c), line1(l1), line2(l2)
	{}

	inline void Set(float c, intptr_t l1, intptr_t l2)
	{
		conv = c;
		line1 = l1;
		line2 = l2;
	}

	inline bool operator<(const LinesConv& rhs) const
	{
		return ((conv > rhs.conv) ||
				((conv == rhs.conv) && ((line2 < rhs.line2) ||
										((line2 == rhs.line2) && (line1 < rhs.line1)))));
	}
};

template<typename CharT>
inline uint64_t Hash(uint64_t hval, CharT letter)
{
	hval ^= static_cast<uint64_t>(letter);

	hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);

	return hval;
}


inline intptr_t toDocLine(const DocCmpInfo& doc, intptr_t bdLine)
{
	assert(bdLine >= 0);

	return (bdLine < (intptr_t)doc.lines.size()) ? doc.lines[bdLine].line : (doc.lines.back().line + 1);
}


inline uint64_t getSectionRangeHash(uint64_t hashSeed, std::vector<wchar_t>& sec, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos >= endPos)
		return hashSeed;

	if (options.ignoreCase)
	{
		const wchar_t storedChar = sec[endPos];

		sec[endPos] = L'\0';

		::CharLowerW((LPWSTR)sec.data() + pos);

		sec[endPos] = storedChar;
	}

	for (; pos < endPos; ++pos)
	{
		if (options.ignoreAllSpaces && (sec[pos] == L' ' || sec[pos] == L'\t'))
			continue;

		if (options.ignoreChangedSpaces && (sec[pos] == L' ' || sec[pos] == L'\t'))
		{
			hashSeed = Hash(hashSeed, L' ');

			while (++pos < endPos && (sec[pos] == L' ' || sec[pos] == L'\t'));

			if (pos == endPos)
				return hashSeed;
		}

		hashSeed = Hash(hashSeed, sec[pos]);
	}

	return hashSeed;
}


uint64_t getRegexIgnoreLineHash(int view, intptr_t off, uint64_t hashSeed, int codepage, const std::vector<char>& line,
	const CompareOptions& options)
{
	const int len = static_cast<int>(line.size());

	if (len == 0)
		return hashSeed;

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

#ifndef MULTITHREAD
	LOGD(LOG_ALGO, "line len " + std::to_string(len) + " to wide char len " + std::to_string(wLen) + "\n");
#endif

	boost::regex_iterator<std::vector<wchar_t>::iterator>		rit(wLine.begin(), wLine.end(), *options.ignoreRegex);
	const boost::regex_iterator<std::vector<wchar_t>::iterator>	rend;

	intptr_t mbPos = 0;

	if (options.invertRegex && (rit != rend || !options.inclRegexNomatchLines))
	{
		intptr_t pos = 0;

		for (; rit != rend; ++rit)
		{
#ifndef MULTITHREAD
			LOGD(LOG_ALGO, "pos " + std::to_string(rit->position()) + ", len " + std::to_string(rit->length()) + "\n");
#endif
			hashSeed = getSectionRangeHash(hashSeed, wLine, rit->position(), rit->position() + rit->length(), options);

			if (options.highlightRegexIgnores)
			{
				const int mbLen = ::WideCharToMultiByte(codepage, 0,
						wLine.data() + pos, static_cast<int>(rit->position() - pos), NULL, 0, NULL, NULL);

				markTextAsChanged(view, off + mbPos, mbLen, Settings.colors().blank);

				pos = rit->position() + rit->length();
				mbPos += mbLen + ::WideCharToMultiByte(codepage, 0,
						wLine.data() + rit->position(), static_cast<int>(rit->length()), NULL, 0, NULL, NULL);
			}
		}

		if (options.highlightRegexIgnores)
			markTextAsChanged(view, off + mbPos, len - 1 - mbPos, Settings.colors().blank);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t highlightPos = 0;
		intptr_t endPos = wLen - 1;

		if (options.ignoreChangedSpaces && (wLine[pos] == L' ' || wLine[pos] == L'\t'))
		{
			while (++pos < endPos && (wLine[pos] == L' ' || wLine[pos] == L'\t'));

			if (pos == endPos)
				return hashSeed;
		}

		while (rit != rend)
		{
#ifndef MULTITHREAD
			LOGD(LOG_ALGO, "pos " + std::to_string(rit->position()) + ", len " + std::to_string(rit->length()) + "\n");
#endif
			hashSeed = getSectionRangeHash(hashSeed, wLine, pos, rit->position(), options);

			if (options.highlightRegexIgnores)
			{
				mbPos += ::WideCharToMultiByte(codepage, 0, wLine.data() + highlightPos,
						static_cast<int>(rit->position() - highlightPos), NULL, 0, NULL, NULL);
				const int mbLen = ::WideCharToMultiByte(codepage, 0,
						wLine.data() + rit->position(), static_cast<int>(rit->length()), NULL, 0, NULL, NULL);

				markTextAsChanged(view, off + mbPos, mbLen, Settings.colors().blank);

				mbPos += mbLen;
			}

			highlightPos = pos = rit->position() + rit->length();
			++rit;
		}

		if (options.ignoreChangedSpaces)
		{
			intptr_t eolPos = endPos;
			while (--eolPos >= pos && (wLine[eolPos] == L'\n' || wLine[eolPos] == L'\r'));

			if (++eolPos > pos)
			{
				intptr_t p = eolPos;
				while (--p >= pos && (wLine[p] == L' ' || wLine[p] == L'\t'));

				if (++p > pos)
					hashSeed = getSectionRangeHash(hashSeed, wLine, pos, p, options);
			}

			hashSeed = getSectionRangeHash(hashSeed, wLine, eolPos, endPos, options);
		}
		else
		{
			hashSeed = getSectionRangeHash(hashSeed, wLine, pos, endPos, options);
		}
	}

	return hashSeed;
}


void getLines(DocCmpInfo& doc, const CompareOptions& options)
{
	constexpr int monitorCancelEveryXLine = 1000;

	progress_ptr& progress = ProgressDlg::Get();

	doc.lines.clear();

	intptr_t linesCount = CallScintilla(doc.view, SCI_GETLENGTH, 0, 0);

	if (linesCount)
		linesCount = getLinesCount(doc.view);
	else
		return;

	if (isLineEmpty(doc.view, linesCount - 1))
		--linesCount;

	if ((doc.section.len <= 0) || (doc.section.off + doc.section.len > linesCount))
		doc.section.len = linesCount - doc.section.off;

	progress->SetMaxCount((doc.section.len / monitorCancelEveryXLine) + 1);

	doc.lines.reserve(doc.section.len);

	int cancelCheckCount = monitorCancelEveryXLine;

	// Group ignore options to speed-up per-line checks
	const bool checkForIgnoredLines = !CallScintilla(doc.view, SCI_GETALLLINESVISIBLE, 0, 0) &&
		(options.ignoreFoldedLines || options.ignoreHiddenLines);
	const bool inclEmptyLinesAndEOL = !options.ignoreEOL && !options.ignoreEmptyLines;
	const bool inclEmptyLines =
		!options.ignoreEmptyLines && (!options.ignoreRegex || !options.invertRegex || options.inclRegexNomatchLines);
	const bool inclRegexEmptyLines =
		!options.ignoreEmptyLines && options.ignoreRegex && options.invertRegex && options.inclRegexNomatchLines;

	const int codepage = getCodepage(doc.view);

	for (intptr_t secLine = 0; secLine < doc.section.len; ++secLine)
	{
		if (!(--cancelCheckCount))
		{
			if (!progress->Advance())
			{
				doc.lines.clear();
				return;
			}

			cancelCheckCount = monitorCancelEveryXLine;
		}

		intptr_t docLine = secLine + doc.section.off;

		if (checkForIgnoredLines)
		{
			if (options.ignoreFoldedLines && getNextLineAfterFold(doc.view, &docLine))
			{
				secLine = --docLine - doc.section.off;
				continue;
			}

			if (options.ignoreHiddenLines && isLineHidden(doc.view, docLine) && !isLineFolded(doc.view, docLine))
			{
				docLine = getUnhiddenLine(doc.view, docLine);
				secLine = --docLine - doc.section.off;
				continue;
			}
		}

		const intptr_t lineStart	= getLineStart(doc.view, docLine);
		const intptr_t lineEndNoEOL	= getLineEnd(doc.view, docLine);
		intptr_t lineEnd;

		if (inclEmptyLinesAndEOL)
		{
			lineEnd = lineStart + CallScintilla(doc.view, SCI_LINELENGTH, docLine, 0);
		}
		else
		{
			lineEnd = lineEndNoEOL;

			// Because of the parent 'if' that check actually means that empty lines are ignored
			if (!options.ignoreEOL)
			{
				if (lineStart == lineEnd)
					continue;
				else
					lineEnd = lineStart + CallScintilla(doc.view, SCI_LINELENGTH, docLine, 0);
			}
		}

		Line newLine {docLine, cHashSeed};

		if (lineStart < lineEnd)
		{
			std::vector<char> line = getText(doc.view, lineStart, lineEnd);

			if (options.ignoreRegex)
			{
#ifndef MULTITHREAD
				LOGD(LOG_ALGO, "Regex Ignore on line " + std::to_string(docLine + 1) +
						", view " + std::to_string(doc.view) + "\n");
#endif
				newLine.hash = getRegexIgnoreLineHash(doc.view, lineStart, newLine.hash, codepage, line, options);

				if (newLine.hash != cHashSeed || inclRegexEmptyLines)
					doc.lines.emplace_back(newLine);
			}
			else
			{
				if (options.ignoreCase)
					toLowerCase(line, codepage);

				intptr_t pos = 0;
				intptr_t endPos = lineEndNoEOL - lineStart;

				if (options.ignoreChangedSpaces)
				{
					while (pos < endPos && (line[pos] == ' ' || line[pos] == '\t'))
						++pos;

					while (--endPos >= pos && (line[endPos] == ' ' || line[endPos] == '\t'));

					++endPos;
				}

				for (; pos < endPos; ++pos)
				{
					if (options.ignoreAllSpaces && (line[pos] == ' ' || line[pos] == '\t'))
						continue;

					if (options.ignoreChangedSpaces && (line[pos] == ' ' || line[pos] == '\t'))
					{
						newLine.hash = Hash(newLine.hash, ' ');

						while (++pos < endPos && (line[pos] == ' ' || line[pos] == '\t'));

						if (pos == endPos)
							break;
					}

					newLine.hash = Hash(newLine.hash, line[pos]);
				}

				if (lineEnd > lineEndNoEOL)
				{
					endPos = lineEnd - lineStart;

					if ((options.ignoreAllSpaces || options.ignoreChangedSpaces) &&
							(line[pos] == ' ' || line[pos] == '\t'))
						while (++pos < endPos && (line[pos] == ' ' || line[pos] == '\t'));

					for (; pos < endPos; ++pos)
						newLine.hash = Hash(newLine.hash, line[pos]);
				}

				if (newLine.hash != cHashSeed || !options.ignoreEmptyLines)
					doc.lines.emplace_back(newLine);
			}
		}
		else if (inclEmptyLines)
		{
			doc.lines.emplace_back(newLine);
		}
	}
}


inline charType getCharTypeW(wchar_t letter)
{
	if (letter == L' ' || letter == L'\t')
		return charType::SPACECHAR;

	if (::IsCharAlphaNumericW(letter) || letter == L'_')
		return charType::ALPHANUMCHAR;

	return charType::OTHERCHAR;
}


inline void recalculateWordPos(int codepage, std::vector<Word>& words, const std::vector<wchar_t>& line)
{
	intptr_t bytePos = 0;
	intptr_t currPos = 0;

	for (auto& word: words)
	{
		if (currPos < word.pos)
			bytePos += ::WideCharToMultiByte(codepage, 0, line.data() + currPos, static_cast<int>(word.pos - currPos),
					NULL, 0, NULL, NULL);

		currPos = word.pos + word.len;
		word.len = ::WideCharToMultiByte(codepage, 0, line.data() + word.pos, static_cast<int>(word.len),
				NULL, 0, NULL, NULL);
		word.pos = bytePos;
		bytePos += word.len;
	}
}


inline void getSectionRangeWords(std::vector<Word>& words, std::vector<wchar_t>& line, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos >= endPos)
		return;

	if (options.ignoreCase)
	{
		const wchar_t storedChar = line[endPos];

		line[endPos] = L'\0';

		::CharLowerW((LPWSTR)line.data() + pos);

		line[endPos] = storedChar;
	}

	charType currentWordType = getCharTypeW(line[pos]);

	Word word {pos, 1};

	if (options.ignoreChangedSpaces && currentWordType == charType::SPACECHAR)
		word.hash = Hash(cHashSeed, L' ');
	else
		word.hash = Hash(cHashSeed, line[pos]);

	for (; ++pos < endPos;)
	{
		const charType newWordType = getCharTypeW(line[pos]);

		if (newWordType == currentWordType)
		{
			++word.len;

			if (currentWordType != charType::SPACECHAR || !options.ignoreChangedSpaces)
				word.hash = Hash(word.hash, line[pos]);
		}
		else
		{
			if (!options.ignoreAllSpaces || currentWordType != charType::SPACECHAR)
				words.emplace_back(word);

			currentWordType = newWordType;

			word.pos = pos;
			word.len = 1;

			if (options.ignoreChangedSpaces && currentWordType == charType::SPACECHAR)
				word.hash = Hash(cHashSeed, L' ');
			else
				word.hash = Hash(cHashSeed, line[pos]);
		}
	}

	if (!options.ignoreAllSpaces || currentWordType != charType::SPACECHAR)
		words.emplace_back(word);
}


std::vector<Word> getRegexIgnoreLineWords(std::vector<wchar_t>& line, const CompareOptions& options)
{
	std::vector<Word> words;

	const intptr_t len = static_cast<intptr_t>(line.size());

	if (len == 0)
		return words;

	boost::regex_iterator<std::vector<wchar_t>::iterator>		rit(line.begin(), line.end(), *options.ignoreRegex);
	const boost::regex_iterator<std::vector<wchar_t>::iterator>	rend;

	if (options.invertRegex && (rit != rend || !options.inclRegexNomatchLines))
	{
		for (; rit != rend; ++rit)
			getSectionRangeWords(words, line, rit->position(), rit->position() + rit->length(), options);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t endPos = len - 1;

		if (options.ignoreChangedSpaces && (line[pos] == L' ' || line[pos] == L'\t'))
		{
			while (++pos < endPos && (line[pos] == L' ' || line[pos] == L'\t'));

			if (pos == endPos)
				return words;
		}

		while (rit != rend)
		{
			getSectionRangeWords(words, line, pos, rit->position(), options);

			pos = rit->position() + rit->length();
			++rit;
		}

		--endPos;

		if (options.ignoreChangedSpaces && (line[endPos] == L' ' || line[endPos] == L'\t'))
		{
			while (--endPos >= pos && (line[endPos] == L' ' || line[endPos] == L'\t'));

			if (endPos < pos)
				return words;
		}

		getSectionRangeWords(words, line, pos, endPos + 1, options);
	}

	return words;
}


std::vector<Word> getLineWords(int view, intptr_t docLine, const CompareOptions& options)
{
	std::vector<Word> words;

	const intptr_t lineStart	= getLineStart(view, docLine);
	const intptr_t lineEnd		= getLineEnd(view, docLine);

	if (lineStart >= lineEnd)
		return words;

	const int codepage = getCodepage(view);

	std::vector<char> line = getText(view, lineStart, lineEnd);

	const int len = static_cast<int>(line.size());

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

	if (options.ignoreRegex)
	{
		words = getRegexIgnoreLineWords(wLine, options);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t endPos = wLen - 1;

		if (options.ignoreChangedSpaces)
		{
			while (pos < endPos && (wLine[pos] == L' ' || wLine[pos] == L'\t'))
				++pos;

			while (--endPos >= pos && (wLine[endPos] == L' ' || wLine[endPos] == L'\t'));

			++endPos;
		}

		getSectionRangeWords(words, wLine, pos, endPos, options);
	}

	// In case of UTF-16 or UTF-32 find words byte positions and lengths because Scintilla uses those
	if (wLen != len)
		recalculateWordPos(codepage, words, wLine);

	return words;
}


inline void recalculateCharPos(int codepage, std::vector<Char>& chars, const std::vector<wchar_t>& sec)
{
	intptr_t bytePos = 0;
	intptr_t currPos = 0;

	for (auto& ch: chars)
	{
		if (currPos < ch.pos)
			bytePos += ::WideCharToMultiByte(codepage, 0, sec.data() + currPos, static_cast<int>(ch.pos - currPos),
					NULL, 0, NULL, NULL);

		currPos = ch.pos + 1;
		const int charLen = ::WideCharToMultiByte(codepage, 0, sec.data() + ch.pos, 1, NULL, 0, NULL, NULL);
		ch.pos = bytePos;
		bytePos += charLen;
	}
}


inline void getSectionRangeChars(std::vector<Char>& chars, std::vector<wchar_t>& sec, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos >= endPos)
		return;

	if (options.ignoreCase)
	{
		const wchar_t storedChar = sec[endPos];

		sec[endPos] = L'\0';

		::CharLowerW((LPWSTR)sec.data() + pos);

		sec[endPos] = storedChar;
	}

	for (; pos < endPos; ++pos)
	{
		const charType typeOfChar = getCharTypeW(sec[pos]);

		if (options.ignoreAllSpaces && typeOfChar == charType::SPACECHAR)
			continue;

		if (options.ignoreChangedSpaces && typeOfChar == charType::SPACECHAR)
		{
			chars.emplace_back(L' ', pos);

			while (++pos < endPos && getCharTypeW(sec[pos]) == charType::SPACECHAR);

			if (pos == endPos)
				break;
		}

		chars.emplace_back(sec[pos], pos);
	}
}


std::vector<Char> getSectionChars(int view, intptr_t secStart, intptr_t secEnd, const CompareOptions& options)
{
	std::vector<Char> chars;

	if (secStart >= secEnd)
		return chars;

	const int codepage = getCodepage(view);

	std::vector<char> sec = getText(view, secStart, secEnd);

	const int len = static_cast<int>(sec.size());

	const int wLen = ::MultiByteToWideChar(codepage, 0, sec.data(), len, NULL, 0);

	std::vector<wchar_t> wSec(wLen);

	::MultiByteToWideChar(codepage, 0, sec.data(), len, wSec.data(), wLen);

	chars.reserve(wLen - 1);

	getSectionRangeChars(chars, wSec, 0, wLen - 1, options);

	// In case of UTF-16 or UTF-32 find chars byte positions because Scintilla uses those
	if (wLen != len)
		recalculateCharPos(codepage, chars, wSec);

	return chars;
}


std::vector<Char> getRegexIgnoreLineChars(int view, intptr_t lineStart, intptr_t lineEnd,
	const CompareOptions& options)
{
	std::vector<Char> chars;

	if (lineStart >= lineEnd)
		return chars;

	const int codepage = getCodepage(view);

	std::vector<char> line = getText(view, lineStart, lineEnd);

	const int len = static_cast<int>(line.size());

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

	chars.reserve(wLen - 1);

	boost::regex_iterator<std::vector<wchar_t>::iterator>		rit(wLine.begin(), wLine.end(), *options.ignoreRegex);
	const boost::regex_iterator<std::vector<wchar_t>::iterator>	rend;

	if (options.invertRegex && (rit != rend || !options.inclRegexNomatchLines))
	{
		for (; rit != rend; ++rit)
			getSectionRangeChars(chars, wLine, rit->position(), rit->position() + rit->length(), options);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t endPos = wLen - 1;

		if (options.ignoreChangedSpaces && (wLine[pos] == L' ' || wLine[pos] == L'\t'))
		{
			while (++pos < endPos && (wLine[pos] == L' ' || wLine[pos] == L'\t'));

			if (pos == endPos)
				return chars;
		}

		while (rit != rend)
		{
			getSectionRangeChars(chars, wLine, pos, rit->position(), options);

			pos = rit->position() + rit->length();
			++rit;
		}

		--endPos;

		if (options.ignoreChangedSpaces && (wLine[endPos] == L' ' || wLine[endPos] == L'\t'))
			while (--endPos >= pos && (wLine[endPos] == L' ' || wLine[endPos] == L'\t'));

		if (endPos >= pos)
			getSectionRangeChars(chars, wLine, pos, endPos + 1, options);
	}

	// In case of UTF-16 or UTF-32 find chars byte positions because Scintilla uses those
	if (wLen != len)
		recalculateCharPos(codepage, chars, wLine);

	return chars;
}


std::vector<std::vector<Char>> getLinesChars(const DocCmpInfo& doc, const diffInfo& blockDiff,
		const CompareOptions& options)
{
	std::vector<std::vector<Char>> chars(blockDiff.len);

	for (intptr_t blockLine = 0; blockLine < blockDiff.len; ++blockLine)
	{
		// Don't get moved lines
		if (blockDiff.info.getNextUnmoved(blockLine))
		{
			--blockLine;
			continue;
		}

		const intptr_t docLine		= doc.lines[blockLine + blockDiff.off].line;
		const intptr_t lineStart	= getLineStart(doc.view, docLine);
		const intptr_t lineEnd		= getLineEnd(doc.view, docLine);

		if (lineStart < lineEnd)
		{
			if (options.ignoreRegex)
			{
				chars[blockLine] = getRegexIgnoreLineChars(doc.view, lineStart, lineEnd, options);
			}
			else
			{
				chars[blockLine] = getSectionChars(doc.view, lineStart, lineEnd, options);

				if (options.ignoreChangedSpaces && !chars[blockLine].empty())
				{
					auto itr = chars[blockLine].begin();

					for (; itr != chars[blockLine].end() && (itr->ch == L' ' || itr->ch == L'\t'); ++itr);

					if (itr != chars[blockLine].begin())
						chars[blockLine].erase(chars[blockLine].begin(), itr);

					size_t i = chars[blockLine].size() - 1;

					for (; i >= 0 && (chars[blockLine][i] == L' ' || chars[blockLine][i] == L'\t'); --i);

					if (++i != chars[blockLine].size())
						chars[blockLine].erase(chars[blockLine].begin() + i, chars[blockLine].end());
				}
			}
		}
	}

	return chars;
}


void findUniqueLines(CompareInfo& cmpInfo)
{
	std::unordered_map<uint64_t, std::vector<intptr_t>> doc1LinesMap;

	for (const auto& line: cmpInfo.doc1.lines)
	{
		auto insertPair = doc1LinesMap.emplace(line.hash, std::vector<intptr_t>{line.line});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.line);
	}

	for (const auto& line: cmpInfo.doc2.lines)
	{
		auto doc1it = doc1LinesMap.find(line.hash);

		if (doc1it != doc1LinesMap.end())
		{
			cmpInfo.doc2.nonUniqueLines.emplace(line.line);

			auto insertPair = cmpInfo.doc1.nonUniqueLines.emplace(doc1it->second[0]);
			if (insertPair.second)
			{
				for (size_t j = 1; j < doc1it->second.size(); ++j)
					cmpInfo.doc1.nonUniqueLines.emplace(doc1it->second[j]);
			}
		}
	}
}


// Scan for the best single matching block in the other file
void findBestMatch(const CompareInfo& cmpInfo, const diffInfo& lookupDiff, intptr_t lookupOff, MatchInfo& mi)
{
	mi.matchLen		= 0;
	mi.matchDiff	= nullptr;

	const std::vector<Line>* pLookupLines;
	const std::vector<Line>* pMatchLines;
	diff_type matchType;

	if (lookupDiff.type == diff_type::DIFF_IN_1)
	{
		pLookupLines	= &cmpInfo.doc1.lines;
		pMatchLines		= &cmpInfo.doc2.lines;
		matchType		= diff_type::DIFF_IN_2;
	}
	else
	{
		pLookupLines	= &cmpInfo.doc2.lines;
		pMatchLines		= &cmpInfo.doc1.lines;
		matchType		= diff_type::DIFF_IN_1;
	}

	intptr_t minMatchLen = 1;

	for (const diffInfo& matchDiff: cmpInfo.blockDiffs)
	{
		if (matchDiff.type != matchType || matchDiff.len < minMatchLen)
			continue;

		for (intptr_t matchOff = 0; matchOff < matchDiff.len; ++matchOff)
		{
			if ((*pLookupLines)[lookupDiff.off + lookupOff] != (*pMatchLines)[matchDiff.off + matchOff])
				continue;

			if (matchDiff.info.getNextUnmoved(matchOff))
			{
				if (matchOff >= matchDiff.len)
					break;

				if ((*pLookupLines)[lookupDiff.off + lookupOff] != (*pMatchLines)[matchDiff.off + matchOff])
					continue;
			}

			intptr_t lookupStart	= lookupOff - 1;
			intptr_t matchStart		= matchOff - 1;

			// Check for the beginning of the matched block (containing lookupOff element)
			for (; lookupStart >= 0 && matchStart >= 0 &&
					(*pLookupLines)[lookupDiff.off + lookupStart] == (*pMatchLines)[matchDiff.off + matchStart] &&
					!lookupDiff.info.movedSection(lookupStart) && !matchDiff.info.movedSection(matchStart);
					--lookupStart, --matchStart);

			++lookupStart;
			++matchStart;

			intptr_t lookupEnd	= lookupOff + 1;
			intptr_t matchEnd	= matchOff + 1;

			// Check for the end of the matched block (containing lookupOff element)
			for (; lookupEnd < lookupDiff.len && matchEnd < matchDiff.len &&
					(*pLookupLines)[lookupDiff.off + lookupEnd] == (*pMatchLines)[matchDiff.off + matchEnd] &&
					!lookupDiff.info.movedSection(lookupEnd) && !matchDiff.info.movedSection(matchEnd);
					++lookupEnd, ++matchEnd);

			const intptr_t matchLen = lookupEnd - lookupStart;

			if (mi.matchLen < matchLen)
			{
				mi.lookupOff	= lookupStart;
				mi.matchDiff	= const_cast<diffInfo*>(&matchDiff);
				mi.matchOff		= matchStart;
				mi.matchLen		= matchLen;

				minMatchLen		= matchLen;
				matchOff		= matchEnd - 1;
			}
			else if (mi.matchLen == matchLen)
			{
				mi.matchDiff	= nullptr;
				matchOff		= matchEnd - 1;
			}
		}
	}
}


// Recursively resolve the best match
bool resolveMatch(const CompareInfo& cmpInfo, diffInfo& lookupDiff, intptr_t lookupOff, MatchInfo& lookupMi)
{
	bool ret = false;

	if (lookupMi.matchDiff)
	{
		lookupOff = lookupMi.matchOff + (lookupOff - lookupMi.lookupOff);

		MatchInfo reverseMi;
		findBestMatch(cmpInfo, *(lookupMi.matchDiff), lookupOff, reverseMi);

		if ((reverseMi.matchDiff == &lookupDiff) && (reverseMi.matchOff == lookupMi.lookupOff))
		{
			LOGD(LOG_ALGO, "Move match found, len: " + std::to_string(lookupMi.matchLen) + "\n");

			lookupDiff.info.moves.emplace_back(lookupMi.lookupOff, lookupMi.matchLen);
			lookupMi.matchDiff->info.moves.emplace_back(lookupMi.matchOff, lookupMi.matchLen);
			ret = true;
		}
		else if (reverseMi.matchDiff)
		{
			ret = resolveMatch(cmpInfo, *(lookupMi.matchDiff), lookupOff, reverseMi);
			lookupMi.matchLen = 0;
		}
	}

	return ret;
}


void findMoves(CompareInfo& cmpInfo)
{
	LOGD(LOG_ALGO, "FIND MOVES\n");

	bool repeat = true;

	while (repeat)
	{
		repeat = false;

		for (diffInfo& lookupDiff: cmpInfo.blockDiffs)
		{
			if (lookupDiff.type != diff_type::DIFF_IN_1)
				continue;

			LOGD(LOG_ALGO, "Check D1 with off: " + std::to_string(cmpInfo.doc1.lines[lookupDiff.off].line + 1) + "\n");

			// Go through all lookupDiff's elements and check if each is matched
			for (intptr_t lookupEi = 0; lookupEi < lookupDiff.len; ++lookupEi)
			{
				// Skip empty lines (do not show blocks of empty/ignored lines as moved)
				if (cmpInfo.doc1.lines[lookupDiff.off + lookupEi].hash == cHashSeed)
					continue;

				// Skip already detected moves
				if (lookupDiff.info.getNextUnmoved(lookupEi))
				{
					if (lookupEi >= lookupDiff.len)
						break;
				}

				MatchInfo mi;
				findBestMatch(cmpInfo, lookupDiff, lookupEi, mi);

				if (resolveMatch(cmpInfo, lookupDiff, lookupEi, mi))
				{
					repeat = true;

					if (mi.matchLen)
						lookupEi = mi.lookupOff + mi.matchLen - 1;
					else
						--lookupEi;
				}
			}
		}
	}
}


inline intptr_t matchBeginEnd(diffInfo& blockDiff1, diffInfo& blockDiff2,
		const std::vector<Char>& sec1, const std::vector<Char>& sec2,
		intptr_t off1, intptr_t off2, intptr_t end1, intptr_t end2,
		std::function<bool(const wchar_t)>&& charFilter_fn)
{
	const intptr_t minSecSize = std::min(sec1.size(), sec2.size());

	intptr_t startMatch = 0;
	while ((minSecSize > startMatch) && (sec1[startMatch] == sec2[startMatch]) && charFilter_fn(sec1[startMatch].ch))
		++startMatch;

	intptr_t endMatch = 0;
	while ((minSecSize - startMatch > endMatch) &&
			(sec1[sec1.size() - endMatch - 1] == sec2[sec2.size() - endMatch - 1]) &&
			charFilter_fn(sec1[sec1.size() - endMatch - 1].ch))
		++endMatch;

	if (startMatch || endMatch)
	{
		line_section_t change;

		if ((intptr_t)sec1.size() > startMatch + endMatch)
		{
			change.off = off1;
			if (startMatch)
				change.off += sec1[startMatch].pos;

			change.len = (endMatch ?
					sec1[sec1.size() - endMatch - 1].pos + 1 + off1 : end1) - change.off;

			if (change.len > 0)
				blockDiff1.info.changedLines.back().changes.emplace_back(change);
		}

		if ((intptr_t)sec2.size() > startMatch + endMatch)
		{
			change.off = off2;
			if (startMatch)
				change.off += sec2[startMatch].pos;

			change.len = (endMatch ?
					sec2[sec2.size() - endMatch - 1].pos + 1 + off2 : end2) - change.off;

			if (change.len > 0)
				blockDiff2.info.changedLines.back().changes.emplace_back(change);
		}
	}

	return (startMatch + endMatch);
}


void findSubLineMoves(int view1, intptr_t docLine1, int view2, intptr_t docLine2,
	std::vector<line_section_t>& line_changes1, std::vector<line_section_t>& line_changes2)
{
	const std::vector<char> line1 = getText(view1, getLineStart(view1, docLine1), getLineEnd(view1, docLine1));
	const std::vector<char> line2 = getText(view2, getLineStart(view2, docLine2), getLineEnd(view2, docLine2));

	for (auto lc1 = line_changes1.begin(); lc1 != line_changes1.end(); lc1++)
	{
		bool sameSectionFound = false;

		for (auto lc = line_changes1.begin(); lc != line_changes1.end(); lc++)
		{
			if (lc->off == lc1->off || lc->moved)
				continue;

			if (lc->len == lc1->len && std::equal(&line1[lc->off], &line1[lc->off + lc->len], &line1[lc1->off]))
			{
				sameSectionFound = true;
				break;
			}
		}

		if (sameSectionFound)
			continue;

		auto lc2same = line_changes2.end();

		for (auto lc2 = line_changes2.begin(); lc2 != line_changes2.end(); lc2++)
		{
			if (lc2->moved)
				continue;

			if (lc2->len == lc1->len && std::equal(&line2[lc2->off], &line2[lc2->off + lc2->len], &line1[lc1->off]))
			{
				if (lc2same == line_changes2.end())
				{
					lc2same = lc2;
				}
				else
				{
					lc2same = line_changes2.end();
					break;
				}
			}
		}

		if (lc2same != line_changes2.end())
		{
			lc1->moved = true;
			lc2same->moved = true;
		}
	}
}


void compareLines(const DocCmpInfo& doc1, const DocCmpInfo& doc2, diffInfo& blockDiff1, diffInfo& blockDiff2,
		const std::map<intptr_t, intptr_t>& lineMappings, const CompareOptions& options)
{
	for (const auto& lm: lineMappings)
	{
		intptr_t line1 = lm.second;
		intptr_t line2 = lm.first;

		LOGD(LOG_ALGO, "Compare Lines " + std::to_string(doc1.lines[blockDiff1.off + line1].line + 1) + " and " +
				std::to_string(doc2.lines[blockDiff2.off + line2].line + 1) + "\n");

		const std::vector<Word> lineWords1 = getLineWords(doc1.view, doc1.lines[blockDiff1.off + line1].line, options);
		const std::vector<Word> lineWords2 = getLineWords(doc2.view, doc2.lines[blockDiff2.off + line2].line, options);

		// First use word granularity (find matching words) for better precision
		const auto lineDiffs = DiffCalc<Word>(lineWords1, lineWords2)(true, true, true);
		const intptr_t lineDiffsSize = static_cast<intptr_t>(lineDiffs.size());

		PRINT_DIFFS("WORD DIFFS", lineDiffs);

		blockDiff1.info.changedLines.emplace_back(line1);
		blockDiff2.info.changedLines.emplace_back(line2);

		const intptr_t lineOff1 = getLineStart(doc1.view, doc1.lines[line1 + blockDiff1.off].line);
		const intptr_t lineOff2 = getLineStart(doc2.view, doc2.lines[line2 + blockDiff2.off].line);

		intptr_t lineLen1 = 0;
		intptr_t lineLen2 = 0;

		for (const auto& word: lineWords1)
			lineLen1 += word.len;

		for (const auto& word: lineWords2)
			lineLen2 += word.len;

		intptr_t totalLineMatchLen = 0;

		for (intptr_t i = 0; i < lineDiffsSize; ++i)
		{
			const auto& ld = lineDiffs[i];

			if (ld.type == diff_type::DIFF_MATCH)
			{
				for (intptr_t j = 0; j < ld.len; ++j)
					totalLineMatchLen += lineWords1[ld.off + j].len;
			}
			else if (ld.type == diff_type::DIFF_IN_2)
			{
				line_section_t change;

				change.off = lineWords2[ld.off].pos;
				change.len = lineWords2[ld.off + ld.len - 1].pos + lineWords2[ld.off + ld.len - 1].len - change.off;

				blockDiff2.info.changedLines.back().changes.emplace_back(change);
			}
			else
			{
				// Resolve words mismatched DIFF_IN_1 / DIFF_IN_2 pairs to find possible sub-word similarities
				if ((i + 1 < lineDiffsSize) && (lineDiffs[i + 1].type == diff_type::DIFF_IN_2))
				{
					const auto& ld2 = lineDiffs[i + 1];

					intptr_t off1 = lineWords1[ld.off].pos;
					intptr_t end1 = lineWords1[ld.off + ld.len - 1].pos + lineWords1[ld.off + ld.len - 1].len;

					intptr_t off2 = lineWords2[ld2.off].pos;
					intptr_t end2 = lineWords2[ld2.off + ld2.len - 1].pos + lineWords2[ld2.off + ld2.len - 1].len;

					const std::vector<Char> sec1 =
							getSectionChars(doc1.view, off1 + lineOff1, end1 + lineOff1, options);
					const std::vector<Char> sec2 =
							getSectionChars(doc2.view, off2 + lineOff2, end2 + lineOff2, options);

					if (options.detectCharDiffs)
					{
						LOGD(LOG_ALGO, "Compare Sections " +
								std::to_string(off1 + 1) + " to " +
								std::to_string(end1 + 1) + " and " +
								std::to_string(off2 + 1) + " to " +
								std::to_string(end2 + 1) + "\n");

						// Compare changed words
						const auto sectionDiffs = DiffCalc<Char>(sec1, sec2)();

						PRINT_DIFFS("CHAR DIFFS", sectionDiffs);

						intptr_t matchLen = 0;
						intptr_t matchSections = 0;

						for (const auto& sd: sectionDiffs)
						{
							if (sd.type == diff_type::DIFF_MATCH)
							{
								matchLen += sd.len;
								++matchSections;
							}
						}

						if (matchSections)
						{
							LOGD(LOG_ALGO, "Matching sections found: " + std::to_string(matchSections) +
									", matched len: " + std::to_string(matchLen) + "\n");

							// Are similarities a considerable portion of the diff?
							if ((int)((matchLen * 100) / std::max(sec1.size(), sec2.size())) >=
								options.changedResemblPercent)
							{
								for (const auto& sd: sectionDiffs)
								{
									if (sd.type == diff_type::DIFF_IN_1)
									{
										line_section_t change;

										change.off = sec1[sd.off].pos + off1;
										change.len = sec1[sd.off + sd.len - 1].pos + off1 + 1 - change.off;

										blockDiff1.info.changedLines.back().changes.emplace_back(change);
									}
									else if (sd.type == diff_type::DIFF_IN_2)
									{
										line_section_t change;

										change.off = sec2[sd.off].pos + off2;
										change.len = sec2[sd.off + sd.len - 1].pos + off2 + 1 - change.off;

										blockDiff2.info.changedLines.back().changes.emplace_back(change);
									}
								}

								totalLineMatchLen += matchLen;

								LOGD(LOG_ALGO, "Whole section checked for matches\n");

								++i;
								continue;
							}
							// If not, mark only beginning and ending diff section matches
							else
							{
								const intptr_t matches =
										matchBeginEnd(blockDiff1, blockDiff2, sec1, sec2, off1, off2, end1, end2,
											[](const wchar_t) { return true; });

								if (matches)
								{
									totalLineMatchLen += matches;

									++i;
									continue;
								}
							}

							// No matching sections between the lines found - move to next lines
							if (lineDiffsSize == 2)
								break;
						}
					}
					// Always match non-alphabetical characters in the beginning and at the end
					else
					{
						const intptr_t matches =
								matchBeginEnd(blockDiff1, blockDiff2, sec1, sec2, off1, off2, end1, end2,
									[](const wchar_t ch) { return (getCharTypeW(ch) != charType::ALPHANUMCHAR); });

						if (matches)
						{
							totalLineMatchLen += matches;

							++i;
							continue;
						}

						// No matching sections between the lines found - move to next lines
						if (lineDiffsSize == 2)
							break;
					}
				}

				line_section_t change;

				change.off = lineWords1[ld.off].pos;
				change.len = lineWords1[ld.off + ld.len - 1].pos + lineWords1[ld.off + ld.len - 1].len - change.off;

				blockDiff1.info.changedLines.back().changes.emplace_back(change);
			}
		}

		// Not enough portion of the lines matches - consider them totally different
		if (((totalLineMatchLen * 100) / std::max(lineLen1, lineLen2)) < options.changedResemblPercent)
		{
			blockDiff1.info.changedLines.pop_back();
			blockDiff2.info.changedLines.pop_back();
		}
		else if (options.detectSubLineMoves)
		{
			if (!blockDiff1.info.changedLines.back().changes.empty() &&
				!blockDiff2.info.changedLines.back().changes.empty())
					findSubLineMoves(
						doc1.view, doc1.lines[blockDiff1.off + lm.second].line,
						doc2.view, doc2.lines[blockDiff2.off + lm.first].line,
						blockDiff1.info.changedLines.back().changes,
						blockDiff2.info.changedLines.back().changes);
		}
	}
}


std::vector<std::set<LinesConv>> getOrderedConvergence(const DocCmpInfo& doc1, const DocCmpInfo& doc2,
		const diffInfo& blockDiff1, const diffInfo& blockDiff2, const CompareOptions& options)
{
	const std::vector<std::vector<Char>> chunk1 = getLinesChars(doc1, blockDiff1, options);
	const std::vector<std::vector<Char>> chunk2 = getLinesChars(doc2, blockDiff2, options);

	const intptr_t linesCount1 = static_cast<intptr_t>(chunk1.size());
	const intptr_t linesCount2 = static_cast<intptr_t>(chunk2.size());

	std::vector<std::vector<Word>> words2(linesCount2);

	if (!options.detectCharDiffs || !options.ignoreAllSpaces)
	{
		for (intptr_t line2 = 0; line2 < linesCount2; ++line2)
			if (!chunk2[line2].empty())
				words2[line2] = getLineWords(doc2.view, doc2.lines[blockDiff2.off + line2].line, options);
	}

	std::vector<std::set<LinesConv>> lines1Convergence(linesCount1);
	std::vector<std::set<LinesConv>> lines2Convergence(linesCount2);

	progress_ptr& progress = ProgressDlg::Get();

	intptr_t linesProgress = 0;

	for (intptr_t line1 = 0; line1 < linesCount1; ++line1)
	{
		if (chunk1[line1].empty())
		{
			linesProgress += linesCount2;
			continue;
		}

		std::vector<Word> words1;

		for (intptr_t line2 = 0; line2 < linesCount2; ++line2)
		{
			if (chunk2[line2].empty())
			{
				++linesProgress;
				continue;
			}

			const intptr_t minSize = std::min(chunk1[line1].size(), chunk2[line2].size());
			const intptr_t maxSize = std::max(chunk1[line1].size(), chunk2[line2].size());

			if (((minSize * 100) / maxSize) < options.changedResemblPercent)
			{
				++linesProgress;
				continue;
			}

			intptr_t matchesCount	= 0;
			intptr_t longestMatch	= 0;

			if (!options.detectCharDiffs || !options.ignoreAllSpaces)
			{
				if (words1.empty())
					words1 = getLineWords(doc1.view, doc1.lines[blockDiff1.off + line1].line, options);

				const auto wordDiffs = DiffCalc<Word>(words1, words2[line2],
						std::bind(&ProgressDlg::IsCancelled, progress))();

				if (progress->IsCancelled())
					return {};

				const intptr_t wordDiffsSize = static_cast<intptr_t>(wordDiffs.size());

				for (intptr_t i = 0; i < wordDiffsSize; ++i)
				{
					if (wordDiffs[i].type == diff_type::DIFF_MATCH)
					{
						intptr_t matchLen = 0;

						for (intptr_t n = 0; n < wordDiffs[i].len; ++n)
							matchLen += words1[wordDiffs[i].off + n].len;

						matchesCount += matchLen;

						if (matchLen > longestMatch)
							longestMatch = matchLen;
					}
				}
			}
			else
			{
				const auto charDiffs = DiffCalc<Char>(chunk1[line1], chunk2[line2],
						std::bind(&ProgressDlg::IsCancelled, progress))();

				if (progress->IsCancelled())
					return {};

				const intptr_t charDiffsSize = static_cast<intptr_t>(charDiffs.size());

				for (intptr_t i = 0; i < charDiffsSize; ++i)
				{
					if (charDiffs[i].type == diff_type::DIFF_MATCH)
					{
						const intptr_t matchLen = charDiffs[i].len;

						matchesCount += matchLen;

						if (matchLen > longestMatch)
							longestMatch = matchLen;
					}
				}
			}

			if (((matchesCount * 100) / maxSize) >= options.changedResemblPercent)
			{
				const float conv =
						(static_cast<float>(matchesCount) * 100) / maxSize +
						(static_cast<float>(longestMatch) * 100) / maxSize;

				if (!progress->Advance(linesProgress + 1))
					return {};

				linesProgress = 0;

				bool addL1C = false;

				if (lines2Convergence[line2].empty() || (conv == lines2Convergence[line2].begin()->conv))
				{
					LOGD(LOG_CHANGE_ALGO, "Add L2: " + std::to_string(line1) + ", " + std::to_string(line2) +
							", " + std::to_string(conv) + "\n");

					lines2Convergence[line2].emplace(conv, line1, line2);
					addL1C = true;
				}
				else if (conv > lines2Convergence[line2].begin()->conv)
				{
					LOGD(LOG_CHANGE_ALGO, "Replace L2: " + std::to_string(line1) + ", " +
							std::to_string(line2) + ", " + std::to_string(conv) + "\n");

					for (const auto& l2c : lines2Convergence[line2])
					{
						auto& l1c = lines1Convergence[l2c.line1];

						for (auto l1cI = l1c.begin(); l1cI != l1c.end(); ++l1cI)
						{
							LOGD(LOG_CHANGE_ALGO, "Check L1: " + std::to_string(l2c.line1) + ", " +
									std::to_string(l1cI->line2) + ", " +
									std::to_string(l1cI->conv) + "\n");

							if (l1cI->line2 == line2)
							{
								LOGD(LOG_CHANGE_ALGO, "Erase\n");

								l1c.erase(l1cI);
								break;
							}
						}
					}

					lines2Convergence[line2].clear();
					lines2Convergence[line2].emplace(conv, line1, line2);
					addL1C = true;
				}

				if (addL1C)
				{
					if (lines1Convergence[line1].empty() || (conv == lines1Convergence[line1].begin()->conv))
					{
						lines1Convergence[line1].emplace(conv, line1, line2);
					}
					else if (conv > lines1Convergence[line1].begin()->conv)
					{
						lines1Convergence[line1].clear();
						lines1Convergence[line1].emplace(conv, line1, line2);
					}
				}
			}
			else
			{
				if (!progress->Advance(linesProgress + 1))
					return {};

				linesProgress = 0;
			}
		}
	}

	return lines1Convergence;
}


bool compareBlocks(const DocCmpInfo& doc1, const DocCmpInfo& doc2, diffInfo& blockDiff1, diffInfo& blockDiff2,
		const CompareOptions& options)
{
	std::vector<std::set<LinesConv>> orderedLinesConvergence =
			getOrderedConvergence(doc1, doc2, blockDiff1, blockDiff2, options);

	if (ProgressDlg::Get()->IsCancelled())
		return false;

#ifdef DLOG
	for (const auto& oc: orderedLinesConvergence)
	{
		if (!oc.empty())
			LOGD(LOG_ALGO, "Best Matching Lines: " +
					std::to_string(doc1.lines[oc.begin()->line1 + blockDiff1.off].line + 1) + " and " +
					std::to_string(doc2.lines[oc.begin()->line2 + blockDiff2.off].line + 1) + ", Conv (" +
					std::to_string(oc.begin()->conv) + ")\n");
	}
#endif

	std::map<intptr_t, intptr_t> bestLineMappings; // line2 -> line1
	{
		std::vector<std::map<intptr_t, intptr_t>> groupedLines;

		for (const auto& oc: orderedLinesConvergence)
		{
			if (oc.empty())
				continue;

			if (groupedLines.empty())
			{
				auto ocItr = oc.begin();

				groupedLines.emplace_back();
				groupedLines.back().emplace(ocItr->line2, ocItr->line1);

				continue;
			}

			intptr_t addToIdx = -1;

			for (auto ocItr = oc.begin(); ocItr != oc.end(); ++ocItr)
			{
				for (intptr_t i = 0; i < static_cast<intptr_t>(groupedLines.size()); ++i)
				{
					const auto& gl = groupedLines[i];

					if ((ocItr->line2) > (gl.rbegin()->first))
					{
						if (addToIdx == -1)
						{
							addToIdx = i;
						}
						else if (groupedLines[addToIdx].size() < gl.size())
						{
							groupedLines.erase(groupedLines.begin() + addToIdx);
							addToIdx = --i;
						}
						else
						{
							groupedLines.erase(groupedLines.begin() + i);
							--i;
						}
					}
				}

				if (addToIdx != -1)
				{
					auto& gl = groupedLines[addToIdx];
					gl.emplace_hint(gl.end(), ocItr->line2, ocItr->line1);

					break;
				}
			}

			if (addToIdx != -1)
				continue;

			auto ocrItr = oc.rbegin();

			std::map<intptr_t, intptr_t> subGroup;

			for (intptr_t i = 0; i < static_cast<intptr_t>(groupedLines.size()); ++i)
			{
				auto& gl = groupedLines[i];

				auto glResItr = gl.emplace(ocrItr->line2, ocrItr->line1);

				if (glResItr.second)
				{
					std::map<intptr_t, intptr_t> newSubGroup;
					auto sgEndItr = glResItr.first;

					newSubGroup.insert(gl.begin(), ++sgEndItr);
					gl.erase(glResItr.first);

					if (newSubGroup.size() > subGroup.size())
						subGroup = std::move(newSubGroup);
				}
			}

			if (!subGroup.empty())
			{
				groupedLines.emplace_back(std::move(subGroup));

				LOGD(LOG_ALGO, "New lines group (" + std::to_string(groupedLines.size()) + " total). Last lines: " +
						std::to_string(doc1.lines[ocrItr->line1 + blockDiff1.off].line + 1) + " - " +
						std::to_string(doc2.lines[ocrItr->line2 + blockDiff2.off].line + 1) + "\n");
			}
		}

		if (groupedLines.empty())
			return true;

		intptr_t	bestGroupIdx = 0;
		size_t		bestSize = groupedLines[0].size();

		for (intptr_t i = 1; i < static_cast<intptr_t>(groupedLines.size()); ++i)
		{
			if (bestSize < groupedLines[i].size())
			{
				bestSize = groupedLines[i].size();
				bestGroupIdx = i;
			}
		}

		bestLineMappings = std::move(groupedLines[bestGroupIdx]);
	}

	compareLines(doc1, doc2, blockDiff1, blockDiff2, bestLineMappings, options);

	return true;
}


void findSubBlockDiffs(CompareInfo& cmpInfo, const CompareOptions& options)
{
	progress_ptr& progress = ProgressDlg::Get();

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	std::vector<intptr_t> changedBlockIdx;

	intptr_t changedProgressCount = 0;

	// Get changed blocks to sub-compare
	for (intptr_t i = 1; i < blockDiffsSize; ++i)
	{
		if ((cmpInfo.blockDiffs[i].type == diff_type::DIFF_IN_2) &&
				(cmpInfo.blockDiffs[i - 1].type == diff_type::DIFF_IN_1))
		{
			changedProgressCount += cmpInfo.blockDiffs[i].len * cmpInfo.blockDiffs[i - 1].len;
			changedBlockIdx.emplace_back(i++);
		}
	}

	progress->SetMaxCount(changedProgressCount);

	if (changedProgressCount > 10000)
		progress->Show();

#ifdef MULTITHREAD // Do multithreaded block compares

	const int threadsCount = std::thread::hardware_concurrency() - 2;

	if (threadsCount > 0)
	{
		LOGD(LOG_ALL, "Changes detection running on " + std::to_string(threadsCount + 1) + " threads\n");

		std::atomic<size_t> blockIdx {0};

		auto threadFn =
			[&]()
			{
				for (size_t i = blockIdx++; i < changedBlockIdx.size(); i = blockIdx++)
				{
					diffInfo& blockDiff1 = cmpInfo.blockDiffs[changedBlockIdx[i] - 1];
					diffInfo& blockDiff2 = cmpInfo.blockDiffs[changedBlockIdx[i]];

					blockDiff1.info.matchBlock = &blockDiff2;
					blockDiff2.info.matchBlock = &blockDiff1;

					if (!compareBlocks(cmpInfo.doc1, cmpInfo.doc2, blockDiff1, blockDiff2, options))
						return;
				}
			};

		std::vector<std::thread> threads(threadsCount);

		for (auto& th: threads)
			th = std::thread(threadFn);

		threadFn();

		for (auto& th: threads)
			th.join();
	}
	else
	{
		LOGD(LOG_ALL, "Changes detection running on 1 thread\n");

		for (intptr_t i: changedBlockIdx)
		{
			diffInfo& blockDiff1 = cmpInfo.blockDiffs[i - 1];
			diffInfo& blockDiff2 = cmpInfo.blockDiffs[i];

			blockDiff1.info.matchBlock = &blockDiff2;
			blockDiff2.info.matchBlock = &blockDiff1;

			if (!compareBlocks(cmpInfo.doc1, cmpInfo.doc2, blockDiff1, blockDiff2, options))
				break;
		}
	}

#else // Do block compares in single thread

	for (intptr_t i: changedBlockIdx)
	{
		diffInfo& blockDiff1 = cmpInfo.blockDiffs[i - 1];
		diffInfo& blockDiff2 = cmpInfo.blockDiffs[i];

		blockDiff1.info.matchBlock = &blockDiff2;
		blockDiff2.info.matchBlock = &blockDiff1;

		if (!compareBlocks(cmpInfo.doc1, cmpInfo.doc2, blockDiff1, blockDiff2, options))
			break;
	}

#endif // MULTITHREAD
}


void flagMatchingBlocks(CompareInfo& cmpInfo)
{
	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	// Flag changed blocks
	for (intptr_t i = 1; i < blockDiffsSize; ++i)
	{
		if ((cmpInfo.blockDiffs[i].type == diff_type::DIFF_IN_2) &&
				(cmpInfo.blockDiffs[i - 1].type == diff_type::DIFF_IN_1))
		{
			diffInfo& blockDiff1 = cmpInfo.blockDiffs[i - 1];
			diffInfo& blockDiff2 = cmpInfo.blockDiffs[i];

			blockDiff1.info.matchBlock = &blockDiff2;
			blockDiff2.info.matchBlock = &blockDiff1;
		}
	}
}


inline void markLine(int view, intptr_t line, int mark)
{
	CallScintilla(view, SCI_ENSUREVISIBLE, line, 0);
	CallScintilla(view, SCI_SHOWLINES, line, line);
	CallScintilla(view, SCI_MARKERADDSET, line, mark);
}


void markSection(const DocCmpInfo& doc, const diffInfo& bd, const CompareOptions& options)
{
	const intptr_t endOff = doc.section.off + doc.section.len;

	for (intptr_t i = doc.section.off, line = bd.off + doc.section.off; i < endOff; ++i, ++line)
	{
		intptr_t movedLen = bd.info.movedSection(i);

		if (movedLen > doc.section.len)
			movedLen = doc.section.len;

		if (movedLen == 0)
		{
			intptr_t prevLine = doc.lines[line].line + 1;

			for (; (i < endOff) && (bd.info.movedSection(i) == 0); ++i, ++line)
			{
				const intptr_t docLine = doc.lines[line].line;
				const int mark = (doc.nonUniqueLines.find(docLine) == doc.nonUniqueLines.end()) ? doc.blockDiffMask :
						(doc.blockDiffMask == MARKER_MASK_ADDED) ? MARKER_MASK_ADDED_LOCAL : MARKER_MASK_REMOVED_LOCAL;

				markLine(doc.view, docLine, mark);

				if (!options.neverMarkIgnored)
				{
					for (; prevLine < docLine; ++prevLine)
						markLine(doc.view, prevLine, doc.blockDiffMask & MARKER_MASK_LINE);

					prevLine = docLine + 1;
				}
			}

			--i;
			--line;
		}
		else if (movedLen == 1)
		{
			markLine(doc.view, doc.lines[line].line, MARKER_MASK_MOVED_SINGLE);
		}
		else
		{
			markLine(doc.view, doc.lines[line].line, MARKER_MASK_MOVED_BEGIN);

			i += --movedLen;

			intptr_t prevLine = doc.lines[line].line + 1;
			intptr_t endLine = line + movedLen;

			for (++line; line < endLine; ++line)
			{
				const intptr_t docLine = doc.lines[line].line;
				markLine(doc.view, docLine, MARKER_MASK_MOVED_MID);

				if (!options.neverMarkIgnored)
				{
					for (; prevLine < docLine; ++prevLine)
						markLine(doc.view, prevLine, MARKER_MASK_MOVED_MID & MARKER_MASK_LINE);

					prevLine = docLine + 1;
				}
			}

			const intptr_t docLine = doc.lines[line].line;
			markLine(doc.view, docLine, MARKER_MASK_MOVED_END);

			if (!options.neverMarkIgnored)
			{
				for (; prevLine < docLine; ++prevLine)
					markLine(doc.view, prevLine, MARKER_MASK_MOVED_MID & MARKER_MASK_LINE);
			}
		}
	}
}


void markLineDiffs(const CompareInfo& cmpInfo, const diffInfo& bd, intptr_t lineIdx)
{
	intptr_t line = cmpInfo.doc1.lines[bd.off + bd.info.changedLines[lineIdx].line].line;
	intptr_t linePos = getLineStart(cmpInfo.doc1.view, line);
	int color = (cmpInfo.doc1.blockDiffMask == MARKER_MASK_ADDED) ?
			Settings.colors().added_part : Settings.colors().removed_part;

	for (const auto& change: bd.info.changedLines[lineIdx].changes)
		markTextAsChanged(cmpInfo.doc1.view, linePos + change.off, change.len,
						change.moved ? Settings.colors().moved_part : color);

	markLine(cmpInfo.doc1.view, line, cmpInfo.doc1.nonUniqueLines.find(line) == cmpInfo.doc1.nonUniqueLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);

	line = cmpInfo.doc2.lines[bd.info.matchBlock->off + bd.info.matchBlock->info.changedLines[lineIdx].line].line;
	linePos = getLineStart(cmpInfo.doc2.view, line);
	color = (cmpInfo.doc2.blockDiffMask == MARKER_MASK_ADDED) ?
			Settings.colors().added_part : Settings.colors().removed_part;

	for (const auto& change: bd.info.matchBlock->info.changedLines[lineIdx].changes)
		markTextAsChanged(cmpInfo.doc2.view, linePos + change.off, change.len,
						change.moved ? Settings.colors().moved_part : color);

	markLine(cmpInfo.doc2.view, line, cmpInfo.doc2.nonUniqueLines.find(line) == cmpInfo.doc2.nonUniqueLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);
}


bool markAllDiffs(CompareInfo& cmpInfo, const CompareOptions& options, CompareSummary& summary)
{
	clearWindow(MAIN_VIEW, false);
	clearWindow(SUB_VIEW, false);

	progress_ptr& progress = ProgressDlg::Get();

	const intptr_t blockDiffSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	progress->SetMaxCount(blockDiffSize);

	std::pair<intptr_t, intptr_t> alignLines {0, 0};

	AlignmentPair alignPair;

	AlignmentViewData* pMainAlignData	= &alignPair.main;
	AlignmentViewData* pSubAlignData	= &alignPair.sub;

	// Make sure pMainAlignData is linked to doc1
	if (cmpInfo.doc1.view == SUB_VIEW)
		std::swap(pMainAlignData, pSubAlignData);

	for (intptr_t i = 0; i < blockDiffSize; ++i)
	{
		const diffInfo& bd = cmpInfo.blockDiffs[i];

		if (bd.type == diff_type::DIFF_MATCH)
		{
			pMainAlignData->diffMask	= 0;
			pMainAlignData->line		= toDocLine(cmpInfo.doc1, alignLines.first);

			pSubAlignData->diffMask		= 0;
			pSubAlignData->line			= toDocLine(cmpInfo.doc2, alignLines.second);

			summary.alignmentInfo.emplace_back(alignPair);

			// Align all pairs of matching lines
			for (intptr_t j = bd.len - 1; j; --j)
			{
				++alignLines.first;
				++alignLines.second;

				pMainAlignData->line	= cmpInfo.doc1.lines[alignLines.first].line;
				pSubAlignData->line		= cmpInfo.doc2.lines[alignLines.second].line;

				summary.alignmentInfo.emplace_back(alignPair);
			}

			++alignLines.first;
			++alignLines.second;

			summary.match += bd.len;
		}
		else if (bd.type == diff_type::DIFF_IN_2)
		{
			cmpInfo.doc2.section.off = 0;
			cmpInfo.doc2.section.len = bd.len;
			markSection(cmpInfo.doc2, bd, options);

			pMainAlignData->diffMask	= 0;
			pMainAlignData->line		= toDocLine(cmpInfo.doc1, alignLines.first);

			pSubAlignData->diffMask		= cmpInfo.doc2.blockDiffMask;
			pSubAlignData->line			= toDocLine(cmpInfo.doc2, alignLines.second);

			summary.alignmentInfo.emplace_back(alignPair);

			const intptr_t movedLines = bd.info.movedCount();

			summary.diffLines	+= bd.len - movedLines;
			summary.moved		+= movedLines;

			if (cmpInfo.doc2.blockDiffMask == MARKER_MASK_ADDED)
				summary.added += bd.len - movedLines;
			else
				summary.removed += bd.len - movedLines;

			alignLines.second += bd.len;
		}
		else if (bd.type == diff_type::DIFF_IN_1)
		{
			if (bd.info.matchBlock)
			{
				const intptr_t changedLinesCount = static_cast<intptr_t>(bd.info.changedLines.size());

				cmpInfo.doc1.section.off = 0;
				cmpInfo.doc2.section.off = 0;

				for (intptr_t j = 0; j < changedLinesCount; ++j)
				{
					cmpInfo.doc1.section.len = bd.info.changedLines[j].line - cmpInfo.doc1.section.off;
					cmpInfo.doc2.section.len = bd.info.matchBlock->info.changedLines[j].line - cmpInfo.doc2.section.off;

					if (cmpInfo.doc1.section.len || cmpInfo.doc2.section.len)
					{
						pMainAlignData->diffMask	= cmpInfo.doc1.section.len ? cmpInfo.doc1.blockDiffMask : 0;
						pMainAlignData->line		= toDocLine(cmpInfo.doc1, alignLines.first);

						pSubAlignData->diffMask		= cmpInfo.doc2.section.len ? cmpInfo.doc2.blockDiffMask : 0;
						pSubAlignData->line			= toDocLine(cmpInfo.doc2, alignLines.second);

						summary.alignmentInfo.emplace_back(alignPair);

						if (options.neverMarkIgnored && cmpInfo.doc1.section.len && cmpInfo.doc2.section.len)
						{
							std::vector<intptr_t> alignLines1;
							intptr_t maxLines = cmpInfo.doc1.section.len + alignLines.first;

							for (intptr_t l = alignLines.first + 1; l < maxLines; ++l)
							{
								if (cmpInfo.doc1.lines[l].line - cmpInfo.doc1.lines[l - 1].line > 1)
									alignLines1.emplace_back(l);
							}

							if (!alignLines1.empty())
							{
								std::vector<intptr_t> alignLines2;
								maxLines = cmpInfo.doc2.section.len + alignLines.second;

								for (intptr_t l = alignLines.second + 1; l < maxLines; ++l)
								{
									if (cmpInfo.doc2.lines[l].line - cmpInfo.doc2.lines[l - 1].line > 1)
										alignLines2.emplace_back(l);
								}

								maxLines = std::min(alignLines1.size(), alignLines2.size());

								for (intptr_t l = 0; l < maxLines; ++l)
								{
									pMainAlignData->line	= toDocLine(cmpInfo.doc1, alignLines1[l]);
									pSubAlignData->line		= toDocLine(cmpInfo.doc2, alignLines2[l]);

									summary.alignmentInfo.emplace_back(alignPair);
								}
							}
						}

						if (cmpInfo.doc1.section.len)
						{
							markSection(cmpInfo.doc1, bd, options);
							alignLines.first += cmpInfo.doc1.section.len;
						}

						if (cmpInfo.doc2.section.len)
						{
							markSection(cmpInfo.doc2, *bd.info.matchBlock, options);
							alignLines.second += cmpInfo.doc2.section.len;
						}
					}

					pMainAlignData->diffMask	= MARKER_MASK_CHANGED;
					pMainAlignData->line		= toDocLine(cmpInfo.doc1, alignLines.first);

					pSubAlignData->diffMask		= MARKER_MASK_CHANGED;
					pSubAlignData->line			= toDocLine(cmpInfo.doc2, alignLines.second);

					summary.alignmentInfo.emplace_back(alignPair);

					markLineDiffs(cmpInfo, bd, j);

					cmpInfo.doc1.section.off = bd.info.changedLines[j].line + 1;
					cmpInfo.doc2.section.off = bd.info.matchBlock->info.changedLines[j].line + 1;

					++alignLines.first;
					++alignLines.second;
				}

				cmpInfo.doc1.section.len = bd.len - cmpInfo.doc1.section.off;
				cmpInfo.doc2.section.len = bd.info.matchBlock->len - cmpInfo.doc2.section.off;

				if (cmpInfo.doc1.section.len || cmpInfo.doc2.section.len)
				{
					pMainAlignData->diffMask	= cmpInfo.doc1.section.len ? cmpInfo.doc1.blockDiffMask : 0;
					pMainAlignData->line		= toDocLine(cmpInfo.doc1, alignLines.first);

					pSubAlignData->diffMask		= cmpInfo.doc2.section.len ? cmpInfo.doc2.blockDiffMask : 0;
					pSubAlignData->line			= toDocLine(cmpInfo.doc2, alignLines.second);

					summary.alignmentInfo.emplace_back(alignPair);

					if (options.neverMarkIgnored && cmpInfo.doc1.section.len && cmpInfo.doc2.section.len)
					{
						std::vector<intptr_t> alignLines1;
						intptr_t maxLines = cmpInfo.doc1.section.len + alignLines.first;

						for (intptr_t l = alignLines.first + 1; l < maxLines; ++l)
						{
							if (cmpInfo.doc1.lines[l].line - cmpInfo.doc1.lines[l - 1].line > 1)
								alignLines1.emplace_back(l);
						}

						if (!alignLines1.empty())
						{
							std::vector<intptr_t> alignLines2;
							maxLines = cmpInfo.doc2.section.len + alignLines.second;

							for (intptr_t l = alignLines.second + 1; l < maxLines; ++l)
							{
								if (cmpInfo.doc2.lines[l].line - cmpInfo.doc2.lines[l - 1].line > 1)
									alignLines2.emplace_back(l);
							}

							maxLines = std::min(alignLines1.size(), alignLines2.size());

							for (intptr_t l = 0; l < maxLines; ++l)
							{
								pMainAlignData->line	= toDocLine(cmpInfo.doc1, alignLines1[l]);
								pSubAlignData->line		= toDocLine(cmpInfo.doc2, alignLines2[l]);

								summary.alignmentInfo.emplace_back(alignPair);
							}
						}
					}

					if (cmpInfo.doc1.section.len)
					{
						markSection(cmpInfo.doc1, bd, options);
						alignLines.first += cmpInfo.doc1.section.len;
					}

					if (cmpInfo.doc2.section.len)
					{
						markSection(cmpInfo.doc2, *bd.info.matchBlock, options);
						alignLines.second += cmpInfo.doc2.section.len;
					}
				}

				const intptr_t movedLines1 = bd.info.movedCount();
				const intptr_t movedLines2 = bd.info.matchBlock->info.movedCount();

				const intptr_t newLines1 = bd.len - changedLinesCount - movedLines1;
				const intptr_t newLines2 = bd.info.matchBlock->len - changedLinesCount - movedLines2;

				summary.diffLines	+= newLines1 + newLines2 + changedLinesCount;
				summary.changed		+= changedLinesCount;
				summary.moved		+= movedLines1 + movedLines2;

				if (cmpInfo.doc1.blockDiffMask == MARKER_MASK_ADDED)
				{
					summary.added	+= newLines1;
					summary.removed	+= newLines2;
				}
				else
				{
					summary.added	+= newLines2;
					summary.removed	+= newLines1;
				}

				++i;
			}
			else
			{
				cmpInfo.doc1.section.off = 0;
				cmpInfo.doc1.section.len = bd.len;
				markSection(cmpInfo.doc1, bd, options);

				pMainAlignData->diffMask	= cmpInfo.doc1.blockDiffMask;
				pMainAlignData->line		= toDocLine(cmpInfo.doc1, alignLines.first);

				pSubAlignData->diffMask		= 0;
				pSubAlignData->line			= toDocLine(cmpInfo.doc2, alignLines.second);

				summary.alignmentInfo.emplace_back(alignPair);

				const intptr_t movedLines = bd.info.movedCount();

				summary.diffLines	+= bd.len - movedLines;
				summary.moved		+= movedLines;

				if (cmpInfo.doc1.blockDiffMask == MARKER_MASK_ADDED)
					summary.added += bd.len - movedLines;
				else
					summary.removed += bd.len - movedLines;

				alignLines.first += bd.len;
			}
		}

		if (!progress->Advance())
			return false;
	}

	summary.moved /= 2;

	if (!progress->NextPhase())
		return false;

	return true;
}


// Needed to format patch generation data
std::vector<diff_section_t> toDiffSections(const CompareInfo& cmpInfo, const CompareOptions& options)
{
	std::vector<diff_section_t> diffSecs;

	diffSecs.reserve(cmpInfo.blockDiffs.size());

	intptr_t line2		= 0;
	intptr_t docLine1	= options.selectionCompare ? options.selections[0].first : 0;
	intptr_t docLine2	= options.selectionCompare ? options.selections[1].first : 0;

	for (const auto& bd: cmpInfo.blockDiffs)
	{
		if (bd.type == diff_type::DIFF_MATCH)
		{
			line2 += bd.len;

			const intptr_t endLine1 = toDocLine(cmpInfo.doc1, bd.off + bd.len);
			const intptr_t endLine2 = toDocLine(cmpInfo.doc2, line2);

			diffSecs.emplace_back(DiffType::MATCH, docLine1, endLine1 - docLine1, docLine2, endLine2 - docLine2);

			docLine1 = endLine1;
			docLine2 = endLine2;
		}
		else if (bd.type == diff_type::DIFF_IN_1)
		{
			const intptr_t endLine1 = toDocLine(cmpInfo.doc1, bd.off + bd.len - 1) + 1;

			docLine1 = toDocLine(cmpInfo.doc1, bd.off);

			diffSecs.emplace_back(DiffType::IN_1, docLine1, endLine1 - docLine1, docLine2, 0);

			docLine1 = endLine1;
		}
		else // bd.type == diff_type::DIFF_IN_2
		{
			line2 = bd.off + bd.len;

			const intptr_t endLine2 = toDocLine(cmpInfo.doc2, line2 - 1) + 1;

			docLine2 = toDocLine(cmpInfo.doc2, bd.off);

			diffSecs.emplace_back(DiffType::IN_2, docLine1, 0, docLine2, endLine2 - docLine2);

			docLine2 = endLine2;
		}
	}

	return diffSecs;
}


CompareResult runCompare(const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	summary.clear();

	CompareInfo cmpInfo;

	cmpInfo.doc1.view	= MAIN_VIEW;
	cmpInfo.doc2.view	= SUB_VIEW;

	if (options.selectionCompare)
	{
		cmpInfo.doc1.section.off	= options.selections[MAIN_VIEW].first;
		cmpInfo.doc1.section.len	= options.selections[MAIN_VIEW].second - options.selections[MAIN_VIEW].first + 1;

		cmpInfo.doc2.section.off	= options.selections[SUB_VIEW].first;
		cmpInfo.doc2.section.len	= options.selections[SUB_VIEW].second - options.selections[SUB_VIEW].first + 1;
	}

	cmpInfo.doc1.blockDiffMask = (options.newFileViewId == MAIN_VIEW) ? MARKER_MASK_ADDED : MARKER_MASK_REMOVED;
	cmpInfo.doc2.blockDiffMask = (options.newFileViewId == MAIN_VIEW) ? MARKER_MASK_REMOVED : MARKER_MASK_ADDED;

	LOGD_GET_TIME;

	getLines(cmpInfo.doc1, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	getLines(cmpInfo.doc2, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	cmpInfo.blockDiffs = DiffCalc<Line, blockDiffInfo>(cmpInfo.doc1.lines, cmpInfo.doc2.lines,
		std::bind(&ProgressDlg::IsCancelled, progress))(
			true, options.ignoreAllSpaces || options.ignoreChangedSpaces, true, options.syncPoints);

	if (progress->IsCancelled())
		return CompareResult::COMPARE_CANCELLED;

	LOGD_GET_TIME;
	PRINT_DIFFS("COMPARE START - LINE DIFFS", cmpInfo.blockDiffs);

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	if (blockDiffsSize == 0 || (blockDiffsSize == 1 && cmpInfo.blockDiffs[0].type == diff_type::DIFF_MATCH))
		return CompareResult::COMPARE_MATCH;

	findUniqueLines(cmpInfo);

	if (options.detectMoves)
		findMoves(cmpInfo);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	if (options.detectSubBlockDiffs)
		findSubBlockDiffs(cmpInfo, options);
	else
		flagMatchingBlocks(cmpInfo);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	// Make sure we have at least one line in each view so the functions' logic below works properly
	if (cmpInfo.doc1.lines.empty())
		cmpInfo.doc1.lines.emplace_back(0, cHashSeed);
	if (cmpInfo.doc2.lines.empty())
		cmpInfo.doc2.lines.emplace_back(0, cHashSeed);

	// Needed for patch generation
	summary.diff1view		= cmpInfo.doc1.view;
	summary.diffSections	= toDiffSections(cmpInfo, options);

	if (!markAllDiffs(cmpInfo, options, summary))
		return CompareResult::COMPARE_CANCELLED;

	return CompareResult::COMPARE_MISMATCH;
}


CompareResult runFindUnique(const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	summary.clear();

	DocCmpInfo doc1;
	DocCmpInfo doc2;

	doc1.view	= MAIN_VIEW;
	doc2.view	= SUB_VIEW;

	if (options.selectionCompare)
	{
		doc1.section.off	= options.selections[MAIN_VIEW].first;
		doc1.section.len	= options.selections[MAIN_VIEW].second - options.selections[MAIN_VIEW].first + 1;

		doc2.section.off	= options.selections[SUB_VIEW].first;
		doc2.section.len	= options.selections[SUB_VIEW].second - options.selections[SUB_VIEW].first + 1;
	}

	if (options.newFileViewId == MAIN_VIEW)
	{
		doc1.blockDiffMask = MARKER_MASK_ADDED;
		doc2.blockDiffMask = MARKER_MASK_REMOVED;
	}
	else
	{
		doc1.blockDiffMask = MARKER_MASK_REMOVED;
		doc2.blockDiffMask = MARKER_MASK_ADDED;
	}

	getLines(doc1, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	getLines(doc2, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<uint64_t, std::vector<intptr_t>> doc1UniqueLines;

	for (const auto& line: doc1.lines)
	{
		auto insertPair = doc1UniqueLines.emplace(line.hash, std::vector<intptr_t>{line.line});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.line);
	}

	doc1.lines.clear();

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<uint64_t, std::vector<intptr_t>> doc2UniqueLines;

	for (const auto& line: doc2.lines)
	{
		auto insertPair = doc2UniqueLines.emplace(line.hash, std::vector<intptr_t>{line.line});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.line);
	}

	doc2.lines.clear();

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	clearWindow(MAIN_VIEW, false);
	clearWindow(SUB_VIEW, false);

	intptr_t doc1UniqueLinesCount = 0;

	for (const auto& uniqueLine: doc1UniqueLines)
	{
		auto doc2it = doc2UniqueLines.find(uniqueLine.first);

		if (doc2it != doc2UniqueLines.end())
		{
			doc2UniqueLines.erase(doc2it);
			++summary.match;
		}
		else
		{
			for (const auto& line: uniqueLine.second)
			{
				markLine(doc1.view, line, doc1.blockDiffMask);
				++doc1UniqueLinesCount;
			}
		}
	}

	if (doc1UniqueLinesCount == 0 && doc2UniqueLines.empty())
		return CompareResult::COMPARE_MATCH;

	if (doc1.blockDiffMask == MARKER_MASK_ADDED)
		summary.added = doc1UniqueLinesCount;
	else
		summary.removed = doc1UniqueLinesCount;

	for (const auto& uniqueLine: doc2UniqueLines)
	{
		for (const auto& line: uniqueLine.second)
			markLine(doc2.view, line, doc2.blockDiffMask);

		if (doc2.blockDiffMask == MARKER_MASK_ADDED)
			summary.added += uniqueLine.second.size();
		else
			summary.removed += uniqueLine.second.size();
	}

	summary.diffLines = summary.added + summary.removed;

	AlignmentPair align;
	align.main.line	= doc1.section.off;
	align.sub.line	= doc2.section.off;

	summary.alignmentInfo.push_back(align);

	return CompareResult::COMPARE_MISMATCH;
}

}


CompareResult compareViews(const CompareOptions& options, const wchar_t* progressInfo, CompareSummary& summary)
{
	CompareResult result = CompareResult::COMPARE_ERROR;

	if (!progressInfo || !ProgressDlg::Open(progressInfo))
		return CompareResult::COMPARE_ERROR;

	clearChangedIndicatorFull(MAIN_VIEW);
	clearChangedIndicatorFull(SUB_VIEW);

	try
	{
		if (options.findUniqueMode)
			result = runFindUnique(options, summary);
		else
			result = runCompare(options, summary);

		ProgressDlg::Close();

		if (result != CompareResult::COMPARE_MISMATCH)
		{
			clearWindow(MAIN_VIEW);
			clearWindow(SUB_VIEW);
		}
	}
	catch (std::exception& e)
	{
		ProgressDlg::Close();

		clearWindow(MAIN_VIEW);
		clearWindow(SUB_VIEW);

		std::string msg = "Exception occurred: ";
		msg += e.what();

		::MessageBoxA(nppData._nppHandle, msg.c_str(), "ComparePlus", MB_OK | MB_ICONWARNING);
	}
	catch (...)
	{
		ProgressDlg::Close();

		::MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "ComparePlus", MB_OK | MB_ICONWARNING);
	}

	return result;
}
