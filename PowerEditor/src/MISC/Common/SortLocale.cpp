// This file is part of Notepad++ project
// Copyright (C)2025 Randall Joseph Fellmy <software@coises.com>

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


#include "SortLocale.h"

static const SortLocale::Result sortSuccess { 0, "0", L"0" };
static const SortLocale::Result sortNothing { 0, "SortLocaleNothing", L"Nothing to sort." };
static const SortLocale::Result warnMultiple { MB_ICONWARNING, "SortLocaleMultiple", L"Sorting multiple selections is not supported." };
static const SortLocale::Result errorUnknown { MB_ICONERROR, "SortLocaleUnknown", L"The reason the sort failed cannot be determined." };

// The error for exceptions, { MB_ICONERROR, "SortLocaleExcept", exception-message } is built dynamically;
// translations should use "$STR_REPLACE$" as the message, since the message is also passed as the string replacement.

SortLocale::Result SortLocale::sort(ScintillaEditView* sci, bool descending) const
{

	DWORD options = LCMAP_SORTKEY | NORM_LINGUISTIC_CASING;
	if (!caseSensitive) options |= LINGUISTIC_IGNORECASE;
	if (digitsAsNumbers) options |= SORT_DIGITSASNUMBERS;
	if (ignoreDiacritics) options |= LINGUISTIC_IGNOREDIACRITIC;
	if (ignoreSymbols) options |= NORM_IGNORESYMBOLS;

	LPCWSTR locale = localeName.empty() ? LOCALE_NAME_USER_DEFAULT : localeName.data();
	UINT codepage = static_cast<UINT>(sci->execute(SCI_GETCODEPAGE));

	intptr_t lines, startPos, topLine, endPos, bottomLine;
	bool rectangular = false;
	bool noselection = false;
	bool forward = true;
	bool missingEOL = false;

	// Set:
	//    rectangular = true for rectangular and thin selections; false all other times
	//    noselection = true if nothing was selected; false all other times
	//    forward = false if caret is less than anchor; true all other times
	//    missingEOL = true if selection ends includes last line of document with no terminating line ending; otherwise false
	//    topLine and bottomLine = line indices to be sorted, bottomLine included
	//    startPos and endPos = text range of all lines to be sorted, endPos not included
	//    lines = number of lines to be sorted
	//
	// Return warnMultiple if there is a multiple stream selection or sortNothing if there is a selection covering fewer than two lines.

	switch (sci->execute(SCI_GETSELECTIONMODE))
	{
		case SC_SEL_THIN :
		case SC_SEL_RECTANGLE:
			lines = sci->execute(SCI_GETSELECTIONS);
			if (lines < 2) return sortNothing;
			if (sci->execute(SCI_GETSELECTIONEMPTY)) noselection = true;
			rectangular = true;
			{
				intptr_t rsa = sci->execute(SCI_GETRECTANGULARSELECTIONANCHOR);
				intptr_t rsc = sci->execute(SCI_GETRECTANGULARSELECTIONCARET);
				forward = rsc > rsa;
				topLine = sci->execute(SCI_LINEFROMPOSITION, forward ? rsa : rsc);
				bottomLine = sci->execute(SCI_LINEFROMPOSITION, forward ? rsc : rsa);
			}
			startPos = sci->execute(SCI_POSITIONFROMLINE, topLine);
			if (bottomLine == sci->execute(SCI_GETLINECOUNT) - 1)  // selection ends on last line of document
			{
				endPos = sci->execute(SCI_GETLENGTH);
				missingEOL = true;
			}
			else
			{
				endPos = sci->execute(SCI_POSITIONFROMLINE, bottomLine + 1);
			}
			break;

		default:
			if (sci->execute(SCI_GETSELECTIONS) != 1) return warnMultiple;
			intptr_t anchor = sci->execute(SCI_GETANCHOR);
			intptr_t caret  = sci->execute(SCI_GETCURRENTPOS);
			if (anchor == caret)
			{
				noselection = true;
				topLine = startPos = 0;
				bottomLine = sci->execute(SCI_GETLINECOUNT) - 1;
				endPos = sci->execute(SCI_GETLENGTH);
				if (sci->execute(SCI_POSITIONFROMLINE, bottomLine) == endPos)  // last line is empty; don't sort it
					bottomLine--;
				else  // last line of document is not empty (no line ending)
					missingEOL = true;
			}
			else
			{
				forward = anchor < caret;
				topLine = sci->execute(SCI_LINEFROMPOSITION, forward ? anchor : caret);
				startPos = sci->execute(SCI_POSITIONFROMLINE, topLine);
				endPos = forward ? caret : anchor;
				bottomLine = sci->execute(SCI_LINEFROMPOSITION, endPos);
				if (sci->execute(SCI_POSITIONFROMLINE, bottomLine) == endPos)  // selection ends at beginning of a line
				{
					bottomLine--;
				}
				else if (bottomLine == sci->execute(SCI_GETLINECOUNT) - 1)  // selection ends in last line of document, with no line ending
				{
					missingEOL = true;
					endPos = sci->execute(SCI_GETLENGTH);
				}
				else  // move end position to include line ending
				{
					endPos = sci->execute(SCI_POSITIONFROMLINE, bottomLine + 1);
				}
			}
			lines = bottomLine - topLine + 1;
			if (lines < 2) return sortNothing;
	}

	// Extensive memory allocation which follows is enclosed in a try block, so failures can be intercepted.
	// No changes are made to the Scintilla document within the try block; failure when changing the document
	// should be caught by the ordinary Notepad++ error capture routines, since the document cannot be recovered.
	// First declare some variables which will be required after the try block is finished.

	std::string sortedText;
	intptr_t cpAnchor = 0;
	intptr_t cpCaret = 0;
	intptr_t vsAnchor = 0;
	intptr_t vsCaret = 0;
	intptr_t lnCaret = 0;

	try
	{

		// Build a vector which will contain the sort keys and pointers to the lines to be sorted.

		struct SortLine {
			std::string key;
			std::string_view content;
			intptr_t index = 0;
			intptr_t lineStart = 0;
			intptr_t lineLength = 0;
			intptr_t keyStart = 0;
			intptr_t keyLength = 0;
			bool appendEOL = false;
		};
		std::vector<SortLine> sortLines(lines);
		if (missingEOL) sortLines.back().appendEOL = true;

		// Get the information we need from Scintilla.

		intptr_t cpNextLine = endPos;
		for (intptr_t n = lines - 1; n >= 0; --n)
		{
			SortLine& sl = sortLines[n];
			if (rectangular)
			{
				sl.index = forward ? n : lines - 1 - n;
				sl.lineStart = sci->execute(SCI_POSITIONFROMLINE, topLine + n);
				sl.keyStart = sci->execute(SCI_GETSELECTIONNSTART, sl.index);
				sl.keyLength =
					(noselection ? sci->execute(SCI_GETLINEENDPOSITION, topLine + n) : sci->execute(SCI_GETSELECTIONNEND, sl.index))
					- sl.keyStart;
			}
			else
			{
				sl.index = n;
				sl.lineStart = sl.keyStart = sci->execute(SCI_POSITIONFROMLINE, topLine + n);
				sl.keyLength = sci->execute(SCI_GETLINEENDPOSITION, topLine + n) - sl.keyStart;
			}
			sl.lineLength = cpNextLine - sl.lineStart;
			cpNextLine = sl.lineStart;
		}

		std::string docEOL;
		if (missingEOL)
		{
			auto eolMode = sci->execute(SCI_GETEOLMODE);
			docEOL = eolMode == SC_EOL_CR ? "\r" : eolMode == SC_EOL_LF ? "\n" : "\r\n";
		}

		// Next, get a pointer into Scintilla's buffer for the range encompassing everything to be sorted.
		// Note that this pointer becomes invalid as soon as we access Scintilla again.
		// Then find the sort keys and content for each line, sort as requested, and build the replacement text.

		const char* const textPointer = reinterpret_cast<const char*>(sci->execute(SCI_GETRANGEPOINTER, startPos, endPos - startPos));

		for (SortLine& sl : sortLines)
		{
			sl.content = std::string_view(sl.lineStart - startPos + textPointer, sl.lineLength);
			std::string_view keyText(sl.keyStart - startPos + textPointer, sl.keyLength);
			if (!keyText.empty())
			{
				constexpr unsigned int safeSize = std::numeric_limits<int>::max() / 2;
				size_t textLength = keyText.length();
				int sortableLength = textLength > safeSize ? safeSize : static_cast<int>(textLength);
				int wideLength = MultiByteToWideChar(codepage, 0, keyText.data(), sortableLength, 0, 0);
				std::wstring wideText(wideLength, 0);
				MultiByteToWideChar(codepage, 0, keyText.data(), sortableLength, wideText.data(), wideLength);
				int m = LCMapStringEx(locale, options, wideText.data(), wideLength, 0, 0, 0, 0, 0);
				sl.key.resize(m, 0);
				LCMapStringEx(locale, options, wideText.data(), wideLength, reinterpret_cast<LPWSTR>(sl.key.data()), m, 0, 0, 0);
			}
		}

		if (descending)
			std::stable_sort(sortLines.begin(), sortLines.end(), [](const SortLine& a, const SortLine& b) { return a.key > b.key; });
		else
			std::stable_sort(sortLines.begin(), sortLines.end(), [](const SortLine& a, const SortLine& b) { return a.key < b.key; });

		sortedText.reserve(endPos - startPos + docEOL.length());
		for (SortLine& sl : sortLines)
		{
			sortedText.append(sl.content);
			if (sl.appendEOL) sortedText.append(docEOL);
		}

		if (missingEOL)  // if we added a line ending, remove line ending from last line of sorted text
		{
			if (sortedText.back() == '\n') sortedText.pop_back();
			if (sortedText.back() == '\r') sortedText.pop_back();
		}

		// Before updating Scintilla, get information we will need to restore the selection (as best we can)

		if (rectangular)
		{
			intptr_t ixTop = sortLines.front().index;
			intptr_t ixBottom = sortLines.back().index;
			intptr_t ixAnchor = forward ? ixTop : ixBottom;
			intptr_t ixCaret = forward ? ixBottom : ixTop;
			cpAnchor = sci->execute(SCI_GETSELECTIONNANCHOR, ixAnchor);
			vsAnchor = sci->execute(SCI_GETSELECTIONNANCHORVIRTUALSPACE, ixAnchor);
			cpCaret = sci->execute(SCI_GETSELECTIONNCARET, ixCaret);
			vsCaret = sci->execute(SCI_GETSELECTIONNCARETVIRTUALSPACE, ixCaret);
			cpAnchor -= sci->execute(SCI_POSITIONFROMLINE, sci->execute(SCI_LINEFROMPOSITION, cpAnchor));
			cpCaret -= sci->execute(SCI_POSITIONFROMLINE, sci->execute(SCI_LINEFROMPOSITION, cpCaret));
		}
		else if (noselection)
		{
			cpCaret = sci->execute(SCI_GETCURRENTPOS);
			lnCaret = sci->execute(SCI_LINEFROMPOSITION, cpCaret);
			cpCaret -= sci->execute(SCI_POSITIONFROMLINE, lnCaret);
			for (intptr_t n = 0; n < lines; ++n) if (sortLines[n].index == lnCaret)
			{
				lnCaret = n;
				break;
			}
		}

	}

	catch (const std::exception& e)
	{
		try
		{
			int errlen = MultiByteToWideChar(CP_ACP, 0, e.what(), -1, 0, 0);
			if (errlen < 2) return errorUnknown;
			std::wstring errmsg(errlen - 1, 0);
			MultiByteToWideChar(CP_ACP, 0, e.what(), -1, errmsg.data(), errlen);
			return { MB_ICONERROR, "SortLocaleExcept", errmsg };
		}
		catch (...)
		{
			return errorUnknown;
		}
	}

	catch (...)
	{
		return errorUnknown;
	}

	// Update Scintilla and restore position or selection

	sci->execute(SCI_SETTARGETRANGE, startPos, endPos);
	sci->execute(SCI_SETSTATUS, 0);
	sci->execute(SCI_REPLACETARGET, sortedText.length(), reinterpret_cast<LPARAM>(sortedText.data()));
	int replaceStatus = static_cast<int>(sci->execute(SCI_GETSTATUS));
	sci->execute(SCI_SETSTATUS, 0);
	if (replaceStatus != SC_STATUS_OK && replaceStatus < SC_STATUS_WARN_START)
	{
		struct ScintillaMemory : std::exception
		{
			const char* what() const noexcept override { return "Scintilla ran out of memory while updating the document."; }
		};
		struct ScintillaFail : std::exception
		{
			const char* what() const noexcept override { return "Scintilla was unable to update the document."; }
		};
		SendMessage(sci->getHSelf(), WM_SETREDRAW, FALSE, 0);  // Without this, Scintilla can hang before message is displayed
		if (replaceStatus == SC_STATUS_BADALLOC)
			throw ScintillaMemory();
		else
			throw ScintillaFail();
	}

	if (rectangular)
	{
		cpAnchor += sci->execute(SCI_POSITIONFROMLINE, forward ? topLine : bottomLine);
		cpCaret += sci->execute(SCI_POSITIONFROMLINE, forward ? bottomLine : topLine);
		sci->execute(SCI_SETRECTANGULARSELECTIONANCHOR, cpAnchor);
		sci->execute(SCI_SETRECTANGULARSELECTIONCARET, cpCaret);
		sci->execute(SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE, vsAnchor);
		sci->execute(SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE, vsCaret);
	}
	else if (noselection)
		sci->execute(SCI_GOTOPOS, sci->execute(SCI_POSITIONFROMLINE, lnCaret) + cpCaret);
	else if (forward)
		sci->execute(SCI_SETSEL, sci->execute(SCI_GETTARGETSTART), sci->execute(SCI_GETTARGETEND));
	else
		sci->execute(SCI_SETSEL, sci->execute(SCI_GETTARGETEND), sci->execute(SCI_GETTARGETSTART));

	return sortSuccess;

}