// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "AutoCompletion.h"

#include <windows.h>

#include <shlwapi.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <locale>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SciLexer.h>
#include <Scintilla.h>

#include "Buffer.h"
#include "Common.h"
#include "Notepad_plus_msgs.h"
#include "NppDarkMode.h"
#include "NppXml.h"
#include "Parameters.h"
#include "ScintillaEditView.h"

static constexpr auto FUNC_IMG_ID = 1000;
static constexpr const char* xpmfn[]{
	/* columns rows colors chars-per-pixel */
	"16 16 36 1 ",
	"u c None",
	"  c #131313",
	". c #252525",
	"X c #161616",
	"o c #202020",
	"O c #393939",
	"+ c #242424",
	"@ c #282828",
	"# c #4E4E4E",
	"$ c #343434",
	"% c #5B5B5B",
	"& c #5F5F5F",
	"* c #626262",
	"= c #404040",
	"- c #686868",
	"; c #434343",
	": c #464646",
	"> c #484848",
	", c #494949",
	"< c #515151",
	"1 c #929292",
	"2 c #9B9B9B",
	"3 c #636363",
	"4 c #656565",
	"5 c #AFAFAF",
	"6 c #B7B7B7",
	"7 c #757575",
	"8 c #CDCDCD",
	"9 c #858585",
	"0 c #868686",
	"q c #DDDDDD",
	"w c #E1E1E1",
	"e c #E9E9E9",
	"r c #EEEEEE",
	"t c #959595",
	"y c #F6F6F6",
	/* pixels */
	"uuuuuuuuuuuuuuuu",
	"uuuuu5o.:uuuuuuu",
	"uuuu8 $:.0uuuuuu",
	"uuuu2 uuuuuuuuuu",
	"uuu6$ 46uuuuuuuu",
	"uuuO   Ouuuuuuuu",
	"uuuu;#uuuuuuuuuu",
	"uuuu##u& 3uu<+uu",
	"uuuu#;0.@X0, >uu",
	"uuuu+>uuuoo >uuu",
	"uuuu >uuu* =uuuu",
	"uuuu 2uu, Xotuuu",
	"uuu# 4u< >9 %ouu",
	"u:,#X0uO>uu1 $ u",
	"u- +7uuuuuuuuuuu",
	"uuuuuuuuuuuuuuuu"
};

static constexpr const char* xpmfnDark[]{
	/* columns rows colors chars-per-pixel */
	"16 16 36 1 ",
	"u c None",
	"  c #ECECEC",
	". c #DADADA",
	"X c #E9E9E9",
	"o c #DFDFDF",
	"O c #C6C6C6",
	"+ c #DBDBDB",
	"@ c #D7D7D7",
	"# c #B1B1B1",
	"$ c #CBCBCB",
	"% c #A4A4A4",
	"& c #A0A0A0",
	"* c #9D9D9D",
	"= c #404040",
	"- c #BFBFBF",
	"; c #BCBCBC",
	": c #B9B9B9",
	"> c #B7B7B7",
	", c #494949",
	"< c #B6B6B6",
	"1 c #6D6D6D",
	"2 c #646464",
	"3 c #9C9C9C",
	"4 c #9A9A9A",
	"5 c #505050",
	"6 c #484848",
	"7 c #8A8A8A",
	"8 c #323232",
	"9 c #7A7A7A",
	"0 c #797979",
	"q c #222222",
	"w c #1E1E1E",
	"e c #161616",
	"r c #111111",
	"t c #6A6A6A",
	"y c #090909",
	/* pixels */
	"uuuuuuuuuuuuuuuu",
	"uuuuu5o.:uuuuuuu",
	"uuuu8 $:.0uuuuuu",
	"uuuu2 uuuuuuuuuu",
	"uuu6$ 46uuuuuuuu",
	"uuuO   Ouuuuuuuu",
	"uuuu;#uuuuuuuuuu",
	"uuuu##u& 3uu<+uu",
	"uuuu#;0.@X0, >uu",
	"uuuu+>uuuoo >uuu",
	"uuuu >uuu* =uuuu",
	"uuuu 2uu, Xotuuu",
	"uuu# 4u< >9 %ouu",
	"u:,#X0uO>uu1 $ u",
	"u- +7uuuuuuuuuuu",
	"uuuuuuuuuuuuuuuu"
};

static constexpr auto BOX_IMG_ID = 1001;
static constexpr const char* xpmbox[]{
	/* columns rows colors chars-per-pixel */
	"16 16 33 1 ",
	"r c None",
	"  c #000000",
	". c #030303",
	"X c #101010",
	"o c #181818",
	"O c #202020",
	"+ c #282828",
	"@ c #191919",
	"# c #222222",
	"$ c #252525",
	"% c #484848",
	"& c #505050",
	"* c #606060",
	"= c #444444",
	"- c #474747",
	"; c #505050",
	": c #535353",
	"> c #565656",
	", c #979797",
	"< c #9A9A9A",
	"1 c #9F9F9F",
	"2 c #A7A7A7",
	"3 c #AFAFAF",
	"4 c #B7B7B7",
	"5 c #757575",
	"6 c #767676",
	"7 c #787878",
	"8 c #818181",
	"9 c #D7D7D7",
	"0 c #DFDFDF",
	"q c #E7E7E7",
	"w c #EFEFEF",
	"e c #979797",
	/* pixels */
	"rrrrrrrrrrrrrrrr",
	"rrrrrrrrrrrrrrrr",
	"rrre4;$  *rrrrrr",
	"r4@ $-rr6;X rrrr",
	"r1oX>rrrrr5+  rr",
	"r1*r%OrrrXO*&;rr",
	"r1*rrr==%rrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r3o<rrr$rrr8$;rr",
	"rr7#%rr$rr #;rrr",
	"rrrr,o.$.%:rrrrr",
	"rrrrrr7*7rrrrrrr",
	"rrrrrrrrrrrrrrrr"
	};

static constexpr const char* xpmboxDark[]{
	/* columns rows colors chars-per-pixel */
	"16 16 33 1 ",
	"r c None",
	"  c #FFFFFF",
	". c #FCFCFC",
	"X c #EFEFEF",
	"o c #E7E7E7",
	"O c #DFDFDF",
	"+ c #D7D7D7",
	"@ c #E6E6E6",
	"# c #DDDDDD",
	"$ c #DADADA",
	"% c #B7B7B7",
	"& c #AFAFAF",
	"* c #9F9F9F",
	"= c #BBBBBB",
	"- c #B8B8B8",
	"; c #AFAFAF",
	": c #ACACAC",
	"> c #A9A9A9",
	", c #686868",
	"< c #656565",
	"1 c #606060",
	"2 c #585858",
	"3 c #505050",
	"4 c #484848",
	"5 c #8A8A8A",
	"6 c #898989",
	"7 c #878787",
	"8 c #7E7E7E",
	"9 c #282828",
	"0 c #202020",
	"q c #181818",
	"w c #101010",
	"e c #686868",
	/* pixels */
	"rrrrrrrrrrrrrrrr",
	"rrrrrrrrrrrrrrrr",
	"rrre4;$  *rrrrrr",
	"r4@ $-rr6;X rrrr",
	"r1oX>rrrrr5+  rr",
	"r1*r%OrrrXO*&;rr",
	"r1*rrr==%rrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r1*rrrr$rrrr1;rr",
	"r3o<rrr$rrr8$;rr",
	"rr7#%rr$rr #;rrr",
	"rrrr,o.$.%:rrrrr",
	"rrrrrr7*7rrrrrrr",
	"rrrrrrrrrrrrrrrr"
};

static constexpr size_t tagMaxLen = 256;

static bool isInList(const std::string& word, const std::vector<std::string>& wordArray)
{
	return std::find(wordArray.begin(), wordArray.end(), word) != wordArray.end();
}

static bool isAllDigits(const std::string& str)
{
	static const auto& loc = std::locale::classic();
	return std::all_of(str.begin(), str.end(),
		[](unsigned char c) { return std::isdigit(c, loc); });
}

static void sortInsensitive(std::vector<std::string>& wordArray)
{
	static const auto loc = std::locale("");
	std::sort(
		wordArray.begin(),
		wordArray.end(),
		[](const std::string &a, const std::string &b)
		{
			return lexicographical_compare(
				a.begin(), a.end(),
				b.begin(), b.end(),
				[](const unsigned char &ch1, const unsigned char &ch2)
				{
					return std::toupper(ch1, loc) < std::toupper(ch2, loc);
				}
			);
		}
	);
}


bool AutoCompletion::showAutoComplete(AutocompleteType autocType, bool autoInsert)
{
	if (autocType == autocFunc && !_funcCompletionActive)
		return false;

	// Get beginning of word and complete word

	intptr_t curPos = _pEditView->execute(SCI_GETCURRENTPOS);
	intptr_t startPos = _pEditView->execute(SCI_WORDSTARTPOSITION, curPos, true);

	if (curPos == startPos)
		return false;

	const auto len = static_cast<size_t>(std::abs(curPos - startPos));

	std::string words;

	if (autocType == autocFunc)
	{
		if (len >= _keyWordMaxLen)
			return false;
	}
	else
	{
		static constexpr size_t bufSize = 256;
		if (len >= bufSize)
			return false;

		intptr_t endPos = _pEditView->execute(SCI_WORDENDPOSITION, curPos, true);

		const auto lena = static_cast<size_t>(std::abs(endPos - startPos));
		if (lena >= bufSize)
			return false;

		char beginChars[bufSize]{};
		_pEditView->getGenericText(beginChars, bufSize, startPos, curPos);

		// Get word array containing all words beginning with beginChars, excluding word equal to excludeChars

		std::vector<std::string> wordArray;

		if (autocType == autocWord || autocType == autocFuncAndWord)
		{
			char excludeChars[bufSize]{};
			_pEditView->getGenericText(excludeChars, bufSize, startPos, endPos);
			getWordArray(wordArray, beginChars, excludeChars);
		}

		// Add keywords to word array

		if (autocType == autocFuncBrief || autocType == autocFuncAndWord)
		{
			for (size_t i = 0, kwlen = _keyWordArray.size(); i < kwlen; ++i)

			{
				int compareResult = 0;

				if (_ignoreCase)
				{
					const std::string kwSufix = _keyWordArray[i].substr(0, len);
					compareResult = ::_stricmp(beginChars, kwSufix.c_str());
				}
				else
				{
					compareResult = _keyWordArray[i].compare(0, len, beginChars);
				}

				if (compareResult == 0)
				{
					if (!isInList(_keyWordArray[i], wordArray))
						wordArray.push_back(_keyWordArray[i]);
				}
			}
		}

		if (!wordArray.size())
			return false;

		// Optionally, auto-insert word

		if (autocType == autocWord && wordArray.size() == 1 && autoInsert)
		{
			const size_t typeSeparatorPos = wordArray[0].find("\x1E");

			intptr_t replacedLength = _pEditView->replaceTarget(
				(typeSeparatorPos == std::string::npos) ?
				wordArray[0] :
				wordArray[0].substr(0, typeSeparatorPos),
				startPos, curPos
			);

			_pEditView->execute(SCI_GOTOPOS, startPos + replacedLength);
			return true;
		}

		// Sort word array and convert it to a single string with space-separated words

		if (autocType == autocWord || autocType & autocFuncAndWord)
		{
			if (_ignoreCase)
				sortInsensitive(wordArray);
			else
				sort(wordArray.begin(), wordArray.end());
		}

		for (size_t i = 0, wordArrayLen = wordArray.size(); i < wordArrayLen; ++i)
		{
			words += wordArray[i];
			if (i != wordArrayLen - 1)
				words += " ";
		}
	}

	// Make Scintilla show the autocompletion menu

	if (NppDarkMode::isThemeDark())
	{
		if (!_isFxImageRegisteredDark)
		{
			_pEditView->execute(SCI_CLEARREGISTEREDIMAGES);
			_pEditView->execute(SCI_REGISTERIMAGE, FUNC_IMG_ID, reinterpret_cast<LPARAM>(xpmfnDark));
			_pEditView->execute(SCI_REGISTERIMAGE, BOX_IMG_ID, reinterpret_cast<LPARAM>(xpmboxDark));
			_isFxImageRegistered = false;
			_isFxImageRegisteredDark = true;
		}
	}
	else
	{
		if (!_isFxImageRegistered)
		{
			_pEditView->execute(SCI_CLEARREGISTEREDIMAGES);
			_pEditView->execute(SCI_REGISTERIMAGE, FUNC_IMG_ID, reinterpret_cast<LPARAM>(xpmfn));
			_pEditView->execute(SCI_REGISTERIMAGE, BOX_IMG_ID, reinterpret_cast<LPARAM>(xpmbox));
			_isFxImageRegistered = true;
			_isFxImageRegisteredDark = false;
		}
	}

	_pEditView->execute(SCI_AUTOCSETTYPESEPARATOR, static_cast<WPARAM>('\x1E'));
	_pEditView->execute(SCI_AUTOCSETSEPARATOR, static_cast<WPARAM>(' '));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, _ignoreCase);
	_pEditView->execute(SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR, _ignoreCase);

	if (autocType == autocFunc)
		_pEditView->showAutoCompletion(curPos - startPos, _keyWords);
	else
		_pEditView->showAutoCompletion(curPos - startPos, words);

	return true;
}

bool AutoCompletion::showApiComplete()
{
	return showAutoComplete(autocFunc, false);
}

bool AutoCompletion::showApiAndWordComplete()
{
	return showAutoComplete(autocFuncAndWord, false);
}

void AutoCompletion::getWordArray(std::vector<std::string>& wordArray, const char* beginChars, const char* excludeChars) const
{
	static constexpr size_t bufSize = 256;
	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();

	if (nppGUI._autocIgnoreNumbers && isAllDigits(beginChars))
		return;

	std::string expr = "\\<";
	expr += beginChars;
	expr += "[^ \\t\\n\\r.,;:\"(){}=<>'+!?\\[\\]]+";

	size_t docLength = _pEditView->execute(SCI_GETLENGTH);

	int flags = SCFIND_WORDSTART | SCFIND_REGEXP | SCFIND_POSIX;

	if (!_ignoreCase || (!_funcCompletionActive && _curLang == L_TEXT))
		flags |= SCFIND_MATCHCASE;

	_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
	intptr_t posFind = _pEditView->searchInTarget(expr, 0, docLength);

	const std::string boxId = "\x1E" + std::to_string(BOX_IMG_ID);
	const std::string funcId = "\x1E" + std::to_string(FUNC_IMG_ID);

	while (posFind >= 0)
	{
		intptr_t wordStart = _pEditView->execute(SCI_GETTARGETSTART);
		intptr_t wordEnd = _pEditView->execute(SCI_GETTARGETEND);

		size_t foundTextLen = wordEnd - wordStart;
		if (foundTextLen < bufSize)
		{
			char w[bufSize]{};
			_pEditView->getGenericText(w, bufSize, wordStart, wordEnd);
			if (!excludeChars || (std::strncmp(w, excludeChars, bufSize) != 0))
			{
				if (_funcCompletionActive)
				{
					if (isInList(w + boxId, _keyWordArray))
					{
						if (!isInList(w + boxId, wordArray))
							wordArray.push_back(w + boxId);
					}
					else if (isInList(w + funcId, _keyWordArray))
					{
						if (!isInList(w + funcId, wordArray))
							wordArray.push_back(w + funcId);
					}
					else if (!isInList(w, wordArray))
					{
						wordArray.push_back(w);
					}
				}
				else
				{
					if (!isInList(w, wordArray))
						wordArray.push_back(w);
				}
			}
		}
		posFind = _pEditView->searchInTarget(expr, wordEnd, docLength);
	}
}

static std::wstring addTrailingSlash(const std::wstring& path)
{
	if (path.length() >=1 && path[path.length() - 1] == '\\')
		return path;
	else
		return path + L"\\";
}

static std::wstring removeTrailingSlash(const std::wstring& path)
{
	if (path.length() >= 1 && path[path.length() - 1] == '\\')
		return path.substr(0, path.length() - 1);
	else
		return path;
}


static bool isAllowedBeforeDriveLetter(wchar_t c)
{
	static const auto loc = std::locale("");
	return c == L'\'' || c == L'"' || c == L'(' || std::isspace(c, loc);
}

static bool getRawPath(const std::wstring& input, std::wstring &rawPath_out)
{
	// Try to find a path in the given input.
	// Algorithm: look for a colon. The colon must be preceded by an alphabetic character.
	// The alphabetic character must, in turn, be preceded by nothing, or by whitespace, or by
	// a quotation mark.
	static const auto loc = std::locale("");
	size_t lastOccurrence = input.rfind(L":");
	if (lastOccurrence == std::string::npos) // No match.
		return false;
	else if (lastOccurrence == 0)
		return false;
	else if (!std::isalpha(input[lastOccurrence - 1], loc))
		return false;
	else if (lastOccurrence >= 2 && !isAllowedBeforeDriveLetter(input[lastOccurrence - 2]))
		return false;

	rawPath_out = input.substr(lastOccurrence - 1);
	return true;
}

static bool getPathsForPathCompletion(const std::wstring& input, std::wstring &rawPath_out, std::wstring &pathToMatch_out)
{
	std::wstring rawPath;
	if (! getRawPath(input, rawPath))
	{
		return false;
	}
	else if (doesFileExist(removeTrailingSlash(rawPath).c_str()))
	{
		return false;
	}
	else if (doesDirectoryExist(rawPath.c_str()))
	{
		rawPath_out = rawPath;
		pathToMatch_out = rawPath;
		return true;
	}
	else
	{
		size_t last_occurrence = rawPath.rfind(L"\\");
		if (last_occurrence == std::wstring::npos) // No match.
			return false;
		else
		{
			rawPath_out = rawPath;
			pathToMatch_out = rawPath.substr(0, last_occurrence);
			return true;
		}
	}
}

void AutoCompletion::showPathCompletion()
{
	// Get current line (at most MAX_PATH characters "backwards" from current caret).
	std::wstring currentLine;
	{
		static constexpr intptr_t bufSize = MAX_PATH;
		auto buf = std::string(bufSize + 1, '\0');
		const intptr_t currentPos = _pEditView->execute(SCI_GETCURRENTPOS);
		const auto startPos = std::max<intptr_t>(0, currentPos - bufSize);
		_pEditView->getGenericText(buf.data(), bufSize + 1, startPos, currentPos);

		const auto cp = static_cast<UINT>(_pEditView->execute(SCI_GETCODEPAGE));
		currentLine = string2wstring(buf, cp);
	}

	/* Try to figure out which path the user wants us to complete.
	   We need to know the "raw path", which is what the user actually wrote.
	   But we also need to know which directory to look in (pathToMatch), which might
	   not be the same as what the user wrote. This happens when the user types an
	   incomplete name.
	   For instance: the user wants to autocomplete "C:\Wind", and assuming that no such directory
	   exists, this means we should list all files and directories in C:.
	*/
	std::wstring rawPath, pathToMatch;
	if (! getPathsForPathCompletion(currentLine, rawPath, pathToMatch))
		return;

	// Get all files and directories in the path.
	std::wstring autoCompleteEntries;
	{
		WIN32_FIND_DATA data;
		std::wstring pathToMatchPlusSlash = addTrailingSlash(pathToMatch);
		std::wstring searchString = pathToMatchPlusSlash + L"*.*";
		HANDLE hFind = ::FindFirstFile(searchString.c_str(), &data);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			// Maximum number of entries to show. Without this it appears to the user like Notepad++ hangs when autocompleting
			// some really large directories (c:\windows\winxsys on my system for instance).
			static constexpr unsigned int maxEntries = 2000;
			unsigned int counter = 0;
			do
			{
				if (++counter > maxEntries)
					break;

				if (std::wcscmp(data.cFileName, L".") == 0 || std::wcscmp(data.cFileName, L"..") == 0)
					continue;

				if (!autoCompleteEntries.empty())
					autoCompleteEntries += L"\n";

				autoCompleteEntries += pathToMatchPlusSlash;
				autoCompleteEntries += data.cFileName;
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // If directory, add trailing slash.
					autoCompleteEntries += L"\\";

			} while (::FindNextFile(hFind, &data));
			::FindClose(hFind);
		}
		else
			return;
	}

	// Show autocompletion box.
	_pEditView->execute(SCI_AUTOCSETSEPARATOR, static_cast<WPARAM>('\n'));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, true);
	_pEditView->execute(SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR, true);
	_pEditView->showAutoCompletion(rawPath.length(), wstring2string(autoCompleteEntries, CP_UTF8));
	return;
}

bool AutoCompletion::showWordComplete(bool autoInsert)
{
	return showAutoComplete(autocWord, autoInsert);
}

bool AutoCompletion::showFunctionComplete()
{
	if (!_funcCompletionActive)
		return false;

	if (_funcCalltip.updateCalltip(0, true))
	{
		return true;
	}
	return false;
}

void AutoCompletion::getCloseTag(char* closeTag, size_t closeTagLen, size_t caretPos, bool isHTML) const
{
	if (isHTML)
	{
		// Skip if caretPos is within any scripting language
		size_t style = _pEditView->execute(SCI_GETSTYLEAT, caretPos);
		if (style >= SCE_HJ_START)
			return;
	}

	char prev = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, caretPos - 2));
	char prevprev = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, caretPos - 3));

	// Closing a tag (i.e. "-->") will be ignored
	if (prevprev == '-' && prev == '-')
		return;

	// "<toto/>" and "<toto arg="0" />" will be ignored
	if (prev == '/')
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX;
	_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
	static constexpr std::string_view tag2find = "<[^\\s>]*";

	const intptr_t targetStart = _pEditView->searchInTarget(tag2find, caretPos, 0);

	if (targetStart < 0)
		return;

	intptr_t targetEnd = _pEditView->execute(SCI_GETTARGETEND);
	intptr_t foundTextLen = targetEnd - targetStart;
	if (foundTextLen < 2) // "<>" will be ignored
		return;

	if (foundTextLen > static_cast<intptr_t>(closeTagLen) - 2) // buffer size is not large enough. -2 for '/' & '\0'
		return;

	char tagHead[tagMaxLen]{};
	_pEditView->getText(tagHead, targetStart, targetEnd);

	if (tagHead[1] == '/') // "</toto>" will be ignored
		return;

	if (tagHead[1] == '?') // "<?" (Processing Instructions) will be ignored
		return;

	if (std::strncmp(tagHead, "<!--", 4) == 0) // Comments will be ignored
		return;

	if (isHTML) // for HTML: ignore void elements
	{
		// https://www.w3.org/TR/html5/syntax.html#void-elements
		static constexpr const char* disallowedTags[]{
				"area", "base", "br", "col", "embed", "hr", "img", "input",
				"keygen", "link", "meta", "param", "source", "track", "wbr",
				"!doctype"
			};
		size_t disallowedTagsLen = sizeof(disallowedTags) / sizeof(char *);
		for (size_t i = 0; i < disallowedTagsLen; ++i)
		{
			if (::strnicmp(tagHead + 1, disallowedTags[i], std::strlen(disallowedTags[i])) == 0)
				return;
		}
	}

	closeTag[0] = '<';
	closeTag[1] = '/';
	_pEditView->getText(closeTag + 2, targetStart + 1, targetEnd);
	closeTag[foundTextLen+1] = '>';
	closeTag[foundTextLen+2] = '\0';
}

void InsertedMatchedChars::removeInvalidElements(MatchedCharInserted mci)
{
	if (mci._c == '\n' || mci._c == '\r') // create a new line, so all matched char are invalidated
	{
		_insertedMatchedChars.clear();
	}
	else
	{
		for (int i = static_cast<int>(_insertedMatchedChars.size()) - 1; i >= 0; --i)
		{
			if (_insertedMatchedChars[i]._pos < mci._pos)
			{
				auto posToDetectLine = _pEditView->execute(SCI_LINEFROMPOSITION, mci._pos);
				auto startPosLine = _pEditView->execute(SCI_LINEFROMPOSITION, _insertedMatchedChars[i]._pos);

				if (posToDetectLine != startPosLine) //not in the same line
				{
					_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
				}
			}
			else // current position is before matchedStartSybol Pos
			{
				_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
			}
		}
	}
}

void InsertedMatchedChars::add(MatchedCharInserted mci)
{
	removeInvalidElements(mci);
	_insertedMatchedChars.push_back(mci);
}

// if current pos > matchedStartSybol Pos and current pos is on the same line of matchedStartSybolPos, it'll be checked then removed
// otherwise it is just removed
// return the pos of matchedEndSybol or -1 if matchedEndSybol not found
intptr_t InsertedMatchedChars::search(char startChar, char endChar, size_t posToDetect)
{
	if (isEmpty())
		return -1;
	auto posToDetectLine = _pEditView->execute(SCI_LINEFROMPOSITION, posToDetect);

	for (auto i = static_cast<int>(_insertedMatchedChars.size()) - 1; i >= 0; --i)
	{
		if (_insertedMatchedChars[i]._c == startChar)
		{
			if (_insertedMatchedChars[i]._pos < posToDetect)
			{
				auto startPosLine = _pEditView->execute(SCI_LINEFROMPOSITION, _insertedMatchedChars[i]._pos);
				if (posToDetectLine == startPosLine)
				{
					size_t endPos = _pEditView->execute(SCI_GETLINEENDPOSITION, startPosLine);

					for (auto j = posToDetect; j <= endPos; ++j)
					{
						char aChar = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, j));

						if (aChar != ' ') // non space is not allowed
						{

							if (aChar == endChar) // found it!!!
							{
								_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
								return j;
							}
							else // whichever character, stop searching
							{
								_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
								return -1;
							}
						}
					}
				}
				else // not in the same line
				{
					_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
				}
			}
			else // current position is before matchedStartSybol Pos
			{
				_insertedMatchedChars.erase(_insertedMatchedChars.begin() + i);
			}
		}
	}
	return -1;
}

void AutoCompletion::insertMatchedChars(int character, const MatchedPairConf & matchedPairConf)
{
	const auto ch = static_cast<char>(character);

	const std::vector<std::pair<char, char>>& matchedPairs = matchedPairConf._matchedPairs;
	size_t caretPos = _pEditView->execute(SCI_GETCURRENTPOS);
	const char* matchedChars = nullptr;

	char charPrev = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, caretPos - 2));
	char charNext = static_cast<char>(_pEditView->execute(SCI_GETCHARAT, caretPos));

	bool isCharPrevBlank = (charPrev == ' ' || charPrev == '\t' || charPrev == '\n' || charPrev == '\r' || charPrev == '\0');
	size_t docLen = _pEditView->getCurrentDocLen();
	bool isCharNextBlank = (charNext == ' ' || charNext == '\t' || charNext == '\n' || charNext == '\r' || caretPos == docLen);
	bool isCharNextCloseSymbol = (charNext == ')' || charNext == ']' || charNext == '}');
	bool isInSandwich = (charPrev == '(' && charNext == ')') || (charPrev == '[' && charNext == ']') || (charPrev == '{' && charNext == '}');

	// User defined matched pairs should be checked firstly
	for (size_t i = 0, len = matchedPairs.size(); i < len; ++i)
	{
		if (matchedPairs[i].first == ch)
		{
			if (isCharNextBlank)
			{
				char userMatchedChar[2] = { '\0', '\0' };
				userMatchedChar[0] = matchedPairs[i].second;
				_pEditView->execute(SCI_INSERTTEXT, caretPos, reinterpret_cast<LPARAM>(userMatchedChar));
				return;
			}
		}
	}

	// if there's no user defined matched pair found, continue to check notepad++'s one

	char closeTag[tagMaxLen]{};
	switch (ch)
	{
		case '(':
		{
			if (matchedPairConf._doParentheses)
			{
				if (isCharNextBlank || isCharNextCloseSymbol)
				{
					matchedChars = ")";
					_insertedMatchedChars.add(MatchedCharInserted(ch, caretPos - 1));
				}
			}
			break;
		}

		case '[':
		{
			if (matchedPairConf._doBrackets)
			{
				if (isCharNextBlank || isCharNextCloseSymbol)
				{
					matchedChars = "]";
					_insertedMatchedChars.add(MatchedCharInserted(ch, caretPos - 1));
				}
			}
			break;
		}

		case '{':
		{
			if (matchedPairConf._doCurlyBrackets)
			{
				if (isCharNextBlank || isCharNextCloseSymbol)
				{
					matchedChars = "}";
					_insertedMatchedChars.add(MatchedCharInserted(ch, caretPos - 1));
				}
			}
			break;
		}

		case '"':
		{
			if (matchedPairConf._doDoubleQuotes)
			{
				if (!_insertedMatchedChars.isEmpty())
				{
					intptr_t pos = _insertedMatchedChars.search('"', ch, caretPos);
					if (pos != -1)
					{
						_pEditView->execute(SCI_DELETERANGE, pos, 1);
						_pEditView->execute(SCI_GOTOPOS, pos);
						return;
					}
				}

				if ((isCharPrevBlank && isCharNextBlank) || isInSandwich ||
					(charPrev == '(' && isCharNextBlank) || (isCharPrevBlank && charNext == ')') ||
					(charPrev == '[' && isCharNextBlank) || (isCharPrevBlank && charNext == ']') ||
					(charPrev == '{' && isCharNextBlank) || (isCharPrevBlank && charNext == '}'))
				{
					matchedChars = "\"";
					_insertedMatchedChars.add(MatchedCharInserted(ch, caretPos - 1));
				}
			}
			break;
		}

		case '\'':
		{
			if (matchedPairConf._doQuotes)
			{
				if (!_insertedMatchedChars.isEmpty())
				{
					intptr_t pos = _insertedMatchedChars.search('\'', ch, caretPos);
					if (pos != -1)
					{
						_pEditView->execute(SCI_DELETERANGE, pos, 1);
						_pEditView->execute(SCI_GOTOPOS, pos);
						return;
					}
				}

				if ((isCharPrevBlank && isCharNextBlank) || isInSandwich ||
					(charPrev == '(' && isCharNextBlank) || (isCharPrevBlank && charNext == ')') ||
					(charPrev == '[' && isCharNextBlank) || (isCharPrevBlank && charNext == ']') ||
					(charPrev == '{' && isCharNextBlank) || (isCharPrevBlank && charNext == '}'))
				{
					matchedChars = "'";
					_insertedMatchedChars.add(MatchedCharInserted(ch, caretPos - 1));
				}
			}
			break;
		}

		case '>':
		{
			if (matchedPairConf._doHtmlXmlTag && (_curLang == L_HTML || _curLang == L_XML))
			{
				getCloseTag(closeTag, tagMaxLen, caretPos, _curLang == L_HTML);
				if (closeTag[0] != '\0')
					matchedChars = closeTag;
			}
		}
		break;

		case ')':
		case ']':
		case '}':
		{
			if (!_insertedMatchedChars.isEmpty())
			{
				char startChar;
				if (ch == ')')
				{
					if (!matchedPairConf._doParentheses)
						return;
					startChar = '(';
				}
				else if (ch == ']')
				{
					if (!matchedPairConf._doBrackets)
						return;
					startChar = '[';
				}
				else // if (ch == '}')
				{
					if (!matchedPairConf._doCurlyBrackets)
						return;
					startChar = '{';
				}

				const intptr_t pos = _insertedMatchedChars.search(startChar, ch, caretPos);
				if (pos != -1)
				{
					_pEditView->execute(SCI_DELETERANGE, pos, 1);
					_pEditView->execute(SCI_GOTOPOS, pos);
				}
				return;
			}
			break;
		}

		default:
			if (!_insertedMatchedChars.isEmpty())
				_insertedMatchedChars.removeInvalidElements(MatchedCharInserted(ch, caretPos - 1));
	}

	if (matchedChars)
		_pEditView->execute(SCI_INSERTTEXT, caretPos, reinterpret_cast<LPARAM>(matchedChars));
}


void AutoCompletion::update(int character)
{
	if (!character)
		return;

	const NppGUI& nppGUI = NppParameters::getInstance().getNppGUI();
	if (!_funcCompletionActive && nppGUI._autocStatus == nppGUI.autoc_func)
		return;

	if (nppGUI._funcParams || _funcCalltip.isVisible())
	{
		if (_funcCalltip.updateCalltip(character)) //calltip visible because triggered by autocomplete, set mode
		{
			return;	//only return in case of success, else autocomplete
		}
	}

	//If autocomplete already active, let Scintilla handle it
	if (!nppGUI._autocBrief && _pEditView->execute(SCI_AUTOCACTIVE) != 0)
		return;

	static constexpr int wordSize = 64;
	char s[wordSize]{};
	_pEditView->getWordToCurrentPos(s, wordSize);

	if (std::strlen(s) >= size_t{ nppGUI._autocFromLen })
	{
		if (nppGUI._autocStatus == nppGUI.autoc_word)
			showWordComplete(false);
		else if (nppGUI._autocStatus == nppGUI.autoc_func)
			if (nppGUI._autocBrief)
				showAutoComplete(autocFuncBrief, false);
			else
				showApiComplete();
		else if (nppGUI._autocStatus == nppGUI.autoc_both)
			showApiAndWordComplete();
	}
}

void AutoCompletion::callTipClick(size_t direction)
{
	if (!_funcCompletionActive)
		return;

	if (direction == 1)
	{
		_funcCalltip.showPrevOverload();
	}
	else if (direction == 2)
	{
		_funcCalltip.showNextOverload();
	}
}

bool AutoCompletion::setLanguage(LangType language)
{
	if (_curLang == language && _pXmlFile)
		return true;

	_curLang = language;

	wchar_t path[MAX_PATH];
	::GetModuleFileNameW(nullptr, path, MAX_PATH);
	PathRemoveFileSpec(path);
	wcscat_s(path, L"\\autoCompletion\\");
	wcscat_s(path, getApiFileName());
	wcscat_s(path, L".xml");

	if (_pXmlFile)
		delete _pXmlFile;

	_pXmlFile = new NppXml::NewDocument();
	_funcCompletionActive = NppXml::loadFile(_pXmlFile, path);

	NppXml::Node autoNode{};
	if (_funcCompletionActive)
	{
		_funcCompletionActive = false; //safety
		NppXml::Element node = NppXml::firstChildElement(_pXmlFile, "NotepadPlus");
		if (!node)
			return false;

		autoNode = node = NppXml::firstChildElement(node, "AutoComplete");
		if (!node)
			return false;

		node = NppXml::firstChildElement(node, "KeyWord");
		if (!node)
			return false;

		_xmlKeyword = node;
		_funcCompletionActive = true;
	}

	if (_funcCompletionActive) //try setting up environment
	{
		//setup defaults
		_ignoreCase = true;
		_funcCalltip._start = '(';
		_funcCalltip._stop = ')';
		_funcCalltip._param = ',';
		_funcCalltip._terminal = ';';
		_funcCalltip._ignoreCase = true;
		_funcCalltip._additionalWordChar.clear();

		NppXml::Element elem = NppXml::firstChildElement(autoNode, "Environment");
		if (elem)
		{
			const char* val = nullptr;
			val = NppXml::attribute(elem, "ignoreCase");
			if (val && std::strcmp(val, "no") == 0)
			{
				_ignoreCase = false;
				_funcCalltip._ignoreCase = false;
			}
			val = NppXml::attribute(elem, "startFunc");
			if (val && val[0])
				_funcCalltip._start = val[0];
			val = NppXml::attribute(elem, "stopFunc");
			if (val && val[0])
				_funcCalltip._stop = val[0];
			val = NppXml::attribute(elem, "paramSeparator");
			if (val && val[0])
				_funcCalltip._param = val[0];
			val = NppXml::attribute(elem, "terminal");
			if (val && val[0])
				_funcCalltip._terminal = val[0];
			val = NppXml::attribute(elem, "additionalWordChar");
			if (val && val[0])
				_funcCalltip._additionalWordChar = val;
		}
	}

	if (_funcCompletionActive)
	{
		_funcCalltip.setLanguageXML(_xmlKeyword);
	}
	else
	{
		_funcCalltip.setLanguageXML({});
	}

	_keyWords.clear();
	_keyWordArray.clear();

	if (_funcCompletionActive)
	{
		//Cache the keywords
		//Iterate through all keywords

		for (NppXml::Element funcNode = _xmlKeyword;
			funcNode;
			funcNode = NppXml::nextSiblingElement(funcNode, "KeyWord"))
		{
			const char* name = NppXml::attribute(funcNode, "name");
			if (name)
			{
				const size_t len = std::strlen(name);
				if (len > 0)
				{
					std::string word = name;
					std::string imgid = "\x1E";
					const char* func = NppXml::attribute(funcNode, "func");
					if (func && std::strcmp(func, "yes") == 0)
						imgid += std::to_string(FUNC_IMG_ID);
					else
						imgid += std::to_string(BOX_IMG_ID);
					word += imgid;
					_keyWordArray.push_back(word.c_str());
					if (len > _keyWordMaxLen)
						_keyWordMaxLen = len;
				}
			}
		}

		if (_ignoreCase)
			sortInsensitive(_keyWordArray);
		else
			sort(_keyWordArray.begin(), _keyWordArray.end());

		for (size_t i = 0, len = _keyWordArray.size(); i < len; ++i)
		{
			_keyWords.append(_keyWordArray[i]);
			_keyWords.append(" ");
		}
	}
	return _funcCompletionActive;
}

const wchar_t* AutoCompletion::getApiFileName()
{
	if (_curLang == L_USER)
	{
		const Buffer* currentBuf = _pEditView->getCurrentBuffer();
		if (currentBuf->isUserDefineLangExt())
		{
			return currentBuf->getUserDefineLangName();
		}
	}

	if (_curLang >= L_EXTERNAL && _curLang < NppParameters::getInstance().L_END)
	{
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		return wmc.char2wchar(NppParameters::getInstance().getELCFromIndex(_curLang - L_EXTERNAL)->_name.c_str(), CP_UTF8);
	}

	if (_curLang > L_EXTERNAL)
		_curLang = L_TEXT;

	if (_curLang == L_JAVASCRIPT)
		_curLang = L_JS_EMBEDDED;

	return ScintillaEditView::_langNameInfoArray[_curLang]._langName;
}

COLORREF AutoCompletion::_autocompleteText = RGB(0x00, 0x00, 0x00);
COLORREF AutoCompletion::_autocompleteBg = RGB(0xFF, 0xFF, 0xFF);
COLORREF AutoCompletion::_selectedText = RGB(0xFF, 0xFF, 0xFF);
COLORREF AutoCompletion::_selectedBg = RGB(0x00, 0x78, 0xD7);
COLORREF AutoCompletion::_calltipBg = RGB(0xFF, 0xFF, 0xFF);
COLORREF AutoCompletion::_calltipText = RGB(0x80, 0x80, 0x80);
COLORREF AutoCompletion::_calltipHighlight = RGB(0x00, 0x00, 0x80);

void AutoCompletion::setColour(COLORREF colour2Set, AutocompleteColorIndex i)
{
	switch (i)
	{
		using enum AutoCompletion::AutocompleteColorIndex;
		case autocompleteText:
		{
			_autocompleteText = colour2Set;
			break;
		}

		case autocompleteBg:
		{
			_autocompleteBg = colour2Set;
			break;
		}

		case selectedText:
		{
			_selectedText = colour2Set;
			break;
		}

		case selectedBg:
		{
			_selectedBg = colour2Set;
			break;
		}

		case calltipBg:
		{
			_calltipBg = colour2Set;
			break;
		}

		case calltipText:
		{
			_calltipText = colour2Set;
			break;
		}

		case calltipHighlight:
		{
			_calltipHighlight = colour2Set;
			break;
		}

		default:
			return;
	}
}

void AutoCompletion::drawAutocomplete(const ScintillaEditView* pEditView)
{
	pEditView->setElementColour(SC_ELEMENT_LIST, _autocompleteText);
	pEditView->setElementColour(SC_ELEMENT_LIST_BACK, _autocompleteBg);
	pEditView->setElementColour(SC_ELEMENT_LIST_SELECTED, _selectedText);
	pEditView->setElementColour(SC_ELEMENT_LIST_SELECTED_BACK, _selectedBg);

	pEditView->execute(SCI_CALLTIPSETBACK, _calltipBg);
	pEditView->execute(SCI_CALLTIPSETFORE, _calltipText);
	pEditView->execute(SCI_CALLTIPSETFOREHLT, _calltipHighlight);
}
