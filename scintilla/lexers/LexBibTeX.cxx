// Copyright 2008-2010 Sergiu Dotenco. The License.txt file describes the
// conditions under which this software may be distributed.

/**
 * @file LexBibTeX.cxx
 * @brief General BibTeX coloring scheme.
 * @author Sergiu Dotenco
 * @date April 18, 2009
 */

#include <stdlib.h>
#include <string.h>

#include <cassert>
#include <cctype>

#include <string>
#include <algorithm>
#include <functional>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

namespace {
	bool IsAlphabetic(unsigned int ch)
	{
		return IsASCII(ch) && std::isalpha(ch) != 0;
	}
	bool IsAlphaNumeric(char ch)
	{
	    return IsASCII(ch) && std::isalnum(ch);
	}

	bool EqualCaseInsensitive(const char* a, const char* b)
	{
		return CompareCaseInsensitive(a, b) == 0;
	}

	bool EntryWithoutKey(const char* name)
	{
		return EqualCaseInsensitive(name,"string");
	}

	char GetClosingBrace(char openbrace)
	{
		char result = openbrace;

		switch (openbrace) {
			case '(': result = ')'; break;
			case '{': result = '}'; break;
		}

		return result;
	}

	bool IsEntryStart(char prev, char ch)
	{
		return prev != '\\' && ch == '@';
	}

	bool IsEntryStart(const StyleContext& sc)
	{
		return IsEntryStart(sc.chPrev, sc.ch);
	}

	void ColorizeBibTeX(unsigned start_pos, int length, int /*init_style*/, WordList* keywordlists[], Accessor& styler)
	{
	    WordList &EntryNames = *keywordlists[0];
		bool fold_compact = styler.GetPropertyInt("fold.compact", 1) != 0;

		std::string buffer;
		buffer.reserve(25);

		// We always colorize a section from the beginning, so let's
		// search for the @ character which isn't escaped, i.e. \@
		while (start_pos > 0 && !IsEntryStart(styler.SafeGetCharAt(start_pos - 1),
			styler.SafeGetCharAt(start_pos))) {
			--start_pos; ++length;
		}

		styler.StartAt(start_pos);
		styler.StartSegment(start_pos);

		int current_line = styler.GetLine(start_pos);
		int prev_level = styler.LevelAt(current_line) & SC_FOLDLEVELNUMBERMASK;
		int current_level = prev_level;
		int visible_chars = 0;

		bool in_comment = false ;
		StyleContext sc(start_pos, length, SCE_BIBTEX_DEFAULT, styler);

		bool going = sc.More(); // needed because of a fuzzy end of file state
		char closing_brace = 0;
		bool collect_entry_name = false;

		for (; going; sc.Forward()) {
			if (!sc.More())
				going = false; // we need to go one behind the end of text

			if (in_comment) {
				if (sc.atLineEnd) {
					sc.SetState(SCE_BIBTEX_DEFAULT);
					in_comment = false;
				}
			}
			else {
				// Found @entry
				if (IsEntryStart(sc)) {
					sc.SetState(SCE_BIBTEX_UNKNOWN_ENTRY);
					sc.Forward();
					++current_level;

					buffer.clear();
					collect_entry_name = true;
				}
				else if ((sc.state == SCE_BIBTEX_ENTRY || sc.state == SCE_BIBTEX_UNKNOWN_ENTRY)
					&& (sc.ch == '{' || sc.ch == '(')) {
					// Entry name colorization done
					// Found either a { or a ( after entry's name, e.g. @entry(...) @entry{...}
					// Closing counterpart needs to be stored.
					closing_brace = GetClosingBrace(sc.ch);

					sc.SetState(SCE_BIBTEX_DEFAULT); // Don't colorize { (

					// @string doesn't have any key
					if (EntryWithoutKey(buffer.c_str()))
						sc.ForwardSetState(SCE_BIBTEX_PARAMETER);
					else
						sc.ForwardSetState(SCE_BIBTEX_KEY); // Key/label colorization
				}

				// Need to handle the case where entry's key is empty
				// e.g. @book{,...}
				if (sc.state == SCE_BIBTEX_KEY && sc.ch == ',') {
					// Key/label colorization done
					sc.SetState(SCE_BIBTEX_DEFAULT); // Don't colorize the ,
					sc.ForwardSetState(SCE_BIBTEX_PARAMETER); // Parameter colorization
				}
				else if (sc.state == SCE_BIBTEX_PARAMETER && sc.ch == '=') {
					sc.SetState(SCE_BIBTEX_DEFAULT); // Don't colorize the =
					sc.ForwardSetState(SCE_BIBTEX_VALUE); // Parameter value colorization

					int start = sc.currentPos;

					// We need to handle multiple situations:
					// 1. name"one two {three}"
					// 2. name={one {one two {two}} three}
					// 3. year=2005

					// Skip ", { until we encounter the first alphanumerical character
					while (sc.More() && !(IsAlphaNumeric(sc.ch) || sc.ch == '"' || sc.ch == '{'))
						sc.Forward();

					if (sc.More()) {
						// Store " or {
						char ch = sc.ch;

						// Not interested in alphanumerical characters
						if (IsAlphaNumeric(ch))
							ch = 0;

						int skipped = 0;

						if (ch) {
							// Skip preceding " or { such as in name={{test}}.
							// Remember how many characters have been skipped
							// Make sure that empty values, i.e. "" are also handled correctly
							while (sc.More() && (sc.ch == ch && (ch != '"' || skipped < 1))) {
								sc.Forward();
								++skipped;
							}
						}

						// Closing counterpart for " is the same character
						if (ch == '{')
							ch = '}';

						// We have reached the parameter value
						// In case the open character was a alnum char, skip until , is found
						// otherwise until skipped == 0
						while (sc.More() && (skipped > 0 || (!ch && !(sc.ch == ',' || sc.ch == closing_brace)))) {
							// Make sure the character isn't escaped
							if (sc.chPrev != '\\') {
								// Parameter value contains a { which is the 2nd case described above
								if (sc.ch == '{')
									++skipped; // Remember it
								else if (sc.ch == '}')
									--skipped;
								else if (skipped == 1 && sc.ch == ch && ch == '"') // Don't ignore cases like {"o}
									skipped = 0;
							}

							sc.Forward();
						}
					}

					// Don't colorize the ,
					sc.SetState(SCE_BIBTEX_DEFAULT);

					// Skip until the , or entry's closing closing_brace is found
					// since this parameter might be the last one
					while (sc.More() && !(sc.ch == ',' || sc.ch == closing_brace))
						sc.Forward();

					int state = SCE_BIBTEX_PARAMETER; // The might be more parameters

					// We've reached the closing closing_brace for the bib entry
					// in case no " or {} has been used to enclose the value,
					// as in 3rd case described above
					if (sc.ch == closing_brace) {
						--current_level;
						// Make sure the text between entries is not colored
						// using parameter's style
						state = SCE_BIBTEX_DEFAULT;
					}

					int end = sc.currentPos;
					current_line = styler.GetLine(end);

					// We have possibly skipped some lines, so the folding levels
					// have to be adjusted separately
					for (int i = styler.GetLine(start); i <= styler.GetLine(end); ++i)
						styler.SetLevel(i, prev_level);

					sc.ForwardSetState(state);
				}

				if (sc.state == SCE_BIBTEX_PARAMETER && sc.ch == closing_brace) {
					sc.SetState(SCE_BIBTEX_DEFAULT);
					--current_level;
				}

				// Non escaped % found which represents a comment until the end of the line
				if (sc.chPrev != '\\' && sc.ch == '%') {
					in_comment = true;
					sc.SetState(SCE_BIBTEX_COMMENT);
				}
			}

			if (sc.state == SCE_BIBTEX_UNKNOWN_ENTRY || sc.state == SCE_BIBTEX_ENTRY) {
				if (!IsAlphabetic(sc.ch) && collect_entry_name)
					collect_entry_name = false;

				if (collect_entry_name) {
					buffer += static_cast<char>(tolower(sc.ch));
                    if (EntryNames.InList(buffer.c_str()))
                        sc.ChangeState(SCE_BIBTEX_ENTRY);
                    else
                        sc.ChangeState(SCE_BIBTEX_UNKNOWN_ENTRY);
				}
			}

			if (sc.atLineEnd) {
				int level = prev_level;

				if (visible_chars == 0 && fold_compact)
					level |= SC_FOLDLEVELWHITEFLAG;

				if ((current_level > prev_level))
					level |= SC_FOLDLEVELHEADERFLAG;
				// else if (current_level < prev_level)
				//	level |= SC_FOLDLEVELBOXFOOTERFLAG; // Deprecated

				if (level != styler.LevelAt(current_line)) {
					styler.SetLevel(current_line, level);
				}

				++current_line;
				prev_level = current_level;
				visible_chars = 0;
			}

			if (!isspacechar(sc.ch))
				++visible_chars;
		}

		sc.Complete();

		// Fill in the real level of the next line, keeping the current flags as they will be filled in later
		int flagsNext = styler.LevelAt(current_line) & ~SC_FOLDLEVELNUMBERMASK;
		styler.SetLevel(current_line, prev_level | flagsNext);
	}
}
static const char * const BibTeXWordLists[] = {
            "Entry Names",
            0,
};


LexerModule lmBibTeX(SCLEX_BIBTEX, ColorizeBibTeX, "bib", 0, BibTeXWordLists);

// Entry Names
//    article, book, booklet, conference, inbook,
//    incollection, inproceedings, manual, mastersthesis,
//    misc, phdthesis, proceedings, techreport, unpublished,
//    string, url

