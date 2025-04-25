// This file is part of Notepad++ project
// Copyright (C)2025 Don HO <don.h@free.fr>

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

bool SortLocale::sort(ScintillaEditView* sci) {

    DWORD options = LCMAP_SORTKEY | NORM_LINGUISTIC_CASING;
    if (!caseSensitive) options |= LINGUISTIC_IGNORECASE;
    if (digitsAsNumbers) options |= SORT_DIGITSASNUMBERS;
    if (ignoreDiacritics) options |= LINGUISTIC_IGNOREDIACRITIC;
    if (ignoreSymbols) options |= NORM_IGNORESYMBOLS;

    LPCWSTR locale = localeName.data();
    UINT codepage = static_cast<UINT>(sci->execute(SCI_GETCODEPAGE));

    intptr_t lines, startPos, topLine, endPos, bottomLine;
    bool rectangular = false;
    bool forward = true;
    bool missingEOL = false;

    // Set:
    //    rectangular = true for rectangular selections; false all other times
    //    forward = false for bottom-to-top rectangular selections; true all other times
    //    missingEOL = true if selection ends includes last line of document with no terminating line ending; otherwise false
    //    topLine and bottomLine = line indices to be sorted, bottomLine included
    //    startPos and endPos = text range of all lines to be sorted, endPos not included
    //    lines = number of lines to be sorted
    //
    // Return false if there is a multiple selection other than a rectangular selection,
    // a selection with no content, or a selection covering fewer than two lines.

    switch (sci->execute(SCI_GETSELECTIONMODE)) {
    case SC_SEL_THIN :
        return false;
    case SC_SEL_RECTANGLE:
        lines = sci->execute(SCI_GETSELECTIONS);
        if (lines < 2 || sci->execute(SCI_GETSELECTIONEMPTY)) return false;
        rectangular = true;
        {
            intptr_t rsa = sci->execute(SCI_GETRECTANGULARSELECTIONANCHOR);
            intptr_t rsc = sci->execute(SCI_GETRECTANGULARSELECTIONCARET);
            forward = rsc > rsa;
            topLine = sci->execute(SCI_LINEFROMPOSITION, forward ? rsa : rsc);
            bottomLine = sci->execute(SCI_LINEFROMPOSITION, forward ? rsc : rsa);
        }
        startPos = sci->execute(SCI_POSITIONFROMLINE, topLine);
        if (bottomLine == sci->execute(SCI_GETLINECOUNT) - 1) { // selection ends on last line of document
            endPos = sci->execute(SCI_GETLENGTH);
            missingEOL = true;
        }
        else endPos = sci->execute(SCI_POSITIONFROMLINE, bottomLine + 1);
        break;
    default:
        if (sci->execute(SCI_GETSELECTIONS) != 1) return false;
        if (sci->execute(SCI_GETSELECTIONEMPTY)) {
            topLine = startPos = 0;
            bottomLine = sci->execute(SCI_GETLINECOUNT) - 1;
            endPos = sci->execute(SCI_GETLENGTH);
            if (sci->execute(SCI_POSITIONFROMLINE, bottomLine) == endPos) bottomLine--;  // last line is empty; don't sort it
            else missingEOL = true;  // last line of document is not empty (no line ending)
        }
        else {
            topLine = sci->execute(SCI_LINEFROMPOSITION, sci->execute(SCI_GETSELECTIONSTART));
            startPos = sci->execute(SCI_POSITIONFROMLINE, topLine);
            endPos = sci->execute(SCI_GETSELECTIONEND);
            bottomLine = sci->execute(SCI_LINEFROMPOSITION, endPos);
            if (sci->execute(SCI_POSITIONFROMLINE, bottomLine) == endPos) bottomLine--;  // selection ends at beginning of a line
            else if (bottomLine == sci->execute(SCI_GETLINECOUNT) - 1) { // selection ends in last line of document, with no line ending
                missingEOL = true;
                endPos = sci->execute(SCI_GETLENGTH);
            }
            else endPos = sci->execute(SCI_POSITIONFROMLINE, bottomLine + 1); // move end position to include line ending
        }
        lines = bottomLine - topLine + 1;
        if (lines < 2) return false;
    }

    // Build a vector which will contain the sort keys and pointers to the lines to be sorted.

    struct SortLine {
        std::string key;
        std::string_view content;
        intptr_t lineStart, lineLength, keyStart, keyLength;
        bool appendEOL = false;
    };
    std::vector<SortLine> sortLines(lines);
    if (missingEOL) sortLines.back().appendEOL = true;

    // Get the information we need from Scintilla.

    intptr_t cpNextLine = endPos;
    for (intptr_t n = lines - 1; n >= 0; --n) {
        SortLine& sl = sortLines[n];
        if (rectangular) {
            intptr_t sn = forward ? n : lines - 1 - n;
            sl.lineStart = sci->execute(SCI_POSITIONFROMLINE, topLine + n);
            sl.keyStart = sci->execute(SCI_GETSELECTIONNSTART, sn);
            sl.keyLength = sci->execute(SCI_GETSELECTIONNEND, sn) - sl.keyStart;
        }
        else {
            sl.lineStart = sl.keyStart = sci->execute(SCI_POSITIONFROMLINE, topLine + n);
            sl.keyLength = sci->execute(SCI_GETLINEENDPOSITION, topLine + n) - sl.keyStart;
        }
        sl.lineLength = cpNextLine - sl.lineStart;
        cpNextLine = sl.lineStart;
    }

    std::string docEOL;
    if (missingEOL) {
        auto eolMode = sci->execute(SCI_GETEOLMODE);
        docEOL = eolMode == SC_EOL_CR ? "\r" : eolMode == SC_EOL_LF ? "\n" : "\r\n";
    }

    // Next, get a direct pointer to everything to be sorted.
    // Note that this pointer becomes invalid as soon as we access Scintilla again.
    // Then find the sort keys and content for each line, sort as requested, and build the replacement text.

    const char* const textPointer = reinterpret_cast<const char*>(sci->execute(SCI_GETRANGEPOINTER, startPos, endPos - startPos));

    for (SortLine& sl : sortLines) {
        sl.content = std::string_view(sl.lineStart - startPos + textPointer, sl.lineLength);
        std::string_view keyText(sl.keyStart - startPos + textPointer, sl.keyLength);
        if (!keyText.empty()) {
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

    std::stable_sort(sortLines.begin(), sortLines.end(),
        [this](const SortLine& a, const SortLine& b) { return descending ? a.key > b.key : a.key < b.key; });

    std::string r;

    r.reserve(endPos - startPos + docEOL.length());
    for (SortLine& sl : sortLines) {
        r.append(sl.content);
        if (sl.appendEOL) r.append(docEOL);
    }

    if (missingEOL) /* if we added a line ending, remove line ending from last line of sorted text */ {
        if (r.back() == '\n') r.pop_back();
        if (r.back() == '\r') r.pop_back();
    }

    sci->execute(SCI_SETTARGETRANGE, startPos, endPos);
    sci->execute(SCI_REPLACETARGETMINIMAL, r.length(), reinterpret_cast<LPARAM>(r.data()));

    // cpTop += ss.textStart;
    // cpBottom += data.sci.PositionFromLine(ss.textLine + lines - 1);
    // data.sci.SetRectangularSelectionAnchor(topToBottom ? cpTop : cpBottom);
    // data.sci.SetRectangularSelectionCaret(topToBottom ? cpBottom : cpTop);
    // data.sci.SetRectangularSelectionAnchorVirtualSpace(topToBottom ? vsTop : vsBottom);
    // data.sci.SetRectangularSelectionCaretVirtualSpace(topToBottom ? vsBottom : vsTop);

    return true;

}