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

#include <windows.h>
#include <wchar.h>
#include <commctrl.h>

#include <cstdlib>
#include <vector>
#include <algorithm>

#include "Tools.h"
#include "NppHelpers.h"

#include "icon_added.h"
#include "icon_removed.h"
#include "icon_changed.h"
#include "icon_moved.h"
#include "icon_arrows.h"


extern int gMarginWidth;

int nppBookmarkMarker	= -1;
int indicatorHighlight	= -1;
int marginNum			= -1;


HWND NppToolbarHandleGetter::hNppToolbar = NULL;


HWND NppToolbarHandleGetter::get()
{
	if (hNppToolbar == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, 0);

	return hNppToolbar;
}


BOOL CALLBACK NppToolbarHandleGetter::enumWindowsCB(HWND hwnd, LPARAM )
{
	wchar_t winClassName[64];

	::GetClassNameW(hwnd, winClassName, _countof(winClassName));

	if (!wcscmp(winClassName, TOOLBARCLASSNAME))
	{
		hNppToolbar = hwnd;
		return FALSE;
	}

	return TRUE;
}


HWND NppTabHandleGetter::hNppTab[2] = { NULL, NULL };


HWND NppTabHandleGetter::get(int viewId)
{
	if (::SendMessageW(nppData._nppHandle, NPPM_ISTABBARHIDDEN, 0, 0))
		return NULL;

	const int idx = (viewId == MAIN_VIEW) ? 0 : 1;

	if (hNppTab[idx] == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, idx);

	return hNppTab[idx];
}


BOOL CALLBACK NppTabHandleGetter::enumWindowsCB(HWND hwnd, LPARAM lParam)
{
	wchar_t winClassName[64];

	::GetClassNameW(hwnd, winClassName, _countof(winClassName));

	if (!wcscmp(winClassName, WC_TABCONTROL))
	{
		RECT tabRect;
		RECT viewRect;

		::GetWindowRect(hwnd, &tabRect);
		::GetWindowRect(getView(static_cast<int>(lParam)), &viewRect);

		if ((tabRect.left <= viewRect.left) && (tabRect.top <= viewRect.top) &&
			(tabRect.right >= viewRect.right) && (tabRect.bottom >= viewRect.bottom))
		{
			hNppTab[lParam] = hwnd;
			return FALSE;
		}
	}

	return TRUE;
}


void ViewLocation::save(int view, intptr_t firstLine)
{
	if (view != MAIN_VIEW && view != SUB_VIEW)
	{
		_view = -1;
		return;
	}

	_view		= view;
	_firstLine	= firstLine;

	if (_firstLine < 0)
		_firstLine = getFirstLine(view);

	_visibleLineOffset = getFirstVisibleLineOffset(view, _firstLine);

	LOGD(LOG_SYNC, "Store " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view location, first visible doc line: " + std::to_string(_firstLine + 1) + "\n");
}


bool ViewLocation::restore(bool ensureCaretVisisble) const
{
	if (_view < 0)
		return false;

	const intptr_t firstVisibleLine = getVisibleFromDocLine(_view, _firstLine) - _visibleLineOffset;

	CallScintilla(_view, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);

	if (ensureCaretVisisble)
		CallScintilla(_view, SCI_SCROLLCARET, 0, 0);

	LOGD(LOG_SYNC, "Restore " + std::string(_view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view location, first visible doc line: " +
			std::to_string(getDocLineFromVisible(_view, firstVisibleLine) + 1) + "\n");

	return true;
}


namespace // anonymous namespace
{

const int cBlinkCount		= 3;
const int cBlinkInterval_ms	= 100;

bool compareMode[2]		= { false, false };
int blankStyle[2]		= { 0, 0 };
bool endAtLastLine[2]	= { true, true };
int caretLineColor[2]	= { 0, 0 };
int caretLineLayer[2]	= { 0, 0 };


void defineColor(int type, int color)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINE,	type, SC_MARK_BACKGROUND);
	CallScintilla(MAIN_VIEW,	SCI_MARKERSETBACK,	type, color);

	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINE,	type, SC_MARK_BACKGROUND);
	CallScintilla(SUB_VIEW,		SCI_MARKERSETBACK,	type, color);
}


void defineRgbaSymbol(int type, const unsigned char* rgba)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINERGBAIMAGE,	type, (LPARAM)rgba);
	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINERGBAIMAGE,	type, (LPARAM)rgba);
}


void setTextStyle(int transparency)
{
	constexpr int cMinAlpha = 0;
	constexpr int cMaxAlpha = 100;

	const int alpha = ((100 - transparency) * (cMaxAlpha - cMinAlpha) / 100) + cMinAlpha;

	CallScintilla(MAIN_VIEW, SCI_INDICSETSTYLE,	indicatorHighlight,	INDIC_ROUNDBOX);
	CallScintilla(MAIN_VIEW, SCI_INDICSETFLAGS,	indicatorHighlight,	SC_INDICFLAG_VALUEFORE);
	CallScintilla(MAIN_VIEW, SCI_INDICSETALPHA,	indicatorHighlight,	alpha);

	CallScintilla(SUB_VIEW, SCI_INDICSETSTYLE,	indicatorHighlight,	INDIC_ROUNDBOX);
	CallScintilla(SUB_VIEW, SCI_INDICSETFLAGS,	indicatorHighlight,	SC_INDICFLAG_VALUEFORE);
	CallScintilla(SUB_VIEW, SCI_INDICSETALPHA,	indicatorHighlight,	alpha);
}


void setBlanksStyle(int view, int blankColor)
{
	if (blankStyle[view] == 0)
		blankStyle[view] = static_cast<int>(CallScintilla(view, SCI_ALLOCATEEXTENDEDSTYLES, 1, 0));

	CallScintilla(view, SCI_ANNOTATIONSETSTYLEOFFSET,	blankStyle[view], 0);
	CallScintilla(view, SCI_STYLESETEOLFILLED,			blankStyle[view], 1);
	CallScintilla(view, SCI_STYLESETBACK,				blankStyle[view], blankColor);
	CallScintilla(view, SCI_STYLESETBOLD,				blankStyle[view], true);
	CallScintilla(view, SCI_ANNOTATIONSETVISIBLE,		ANNOTATION_STANDARD, 0);
}

} // anonymous namespace


std::vector<intptr_t> getVisibleLines(int view, bool skipFirstLine)
{
	std::vector<intptr_t> lines;

	intptr_t l = 0;
	intptr_t linesCount = getLinesCount(view);

	const std::pair<intptr_t, intptr_t> sel = getSelectionLines(view);

	if (sel.first >= 0)
	{
		l = sel.first;
		linesCount = sel.second + 1;
	}

	if (l == 0 && skipFirstLine)
		++l;

	for (; l < linesCount; ++l)
		if (!isLineHidden(view, l))
			lines.emplace_back(l);

	return lines;
}


// Make sure you have called at least once readNppBookmarkID() before using that functions!
std::vector<intptr_t> getAllBookmarkedLines(int view)
{
	std::vector<intptr_t> bookmarkedLines;

	const intptr_t	linesCount	= getLinesCount(view);
	intptr_t		line		= CallScintilla(view, SCI_MARKERNEXT, 0, nppBookmarkMarker);

	while (line >= 0)
	{
		bookmarkedLines.emplace_back(line);

		if (++line < linesCount)
			line = CallScintilla(view, SCI_MARKERNEXT, line, nppBookmarkMarker);
		else
			break;
	}

	return bookmarkedLines;
}


void bookmarkMarkedLines(int view, int markMask)
{
	const intptr_t	linesCount	= getLinesCount(view);
	intptr_t		line		= CallScintilla(view, SCI_MARKERNEXT, 0, markMask);

	while (line >= 0)
	{
		CallScintilla(view, SCI_MARKERADDSET, line, nppBookmarkMarker);

		if (++line < linesCount)
			line = CallScintilla(view, SCI_MARKERNEXT, line, markMask);
		else
			break;
	}
}


intptr_t otherViewMatchingLine(int view, intptr_t line, intptr_t adjustment, bool check)
{
	const int		otherView		= getOtherViewId(view);
	const intptr_t	otherLineCount	= getLinesCount(otherView);

	const intptr_t otherLine = getDocLineFromVisible(otherView, getVisibleFromDocLine(view, line) + adjustment);

	if (check && (otherLine < otherLineCount) && (otherViewMatchingLine(otherView, otherLine, -adjustment) != line))
		return -1;

	return (otherLine >= otherLineCount) ? otherLineCount - 1 : otherLine;
}


void activateBufferID(LRESULT buffId)
{
	if (buffId != getCurrentBuffId())
	{
		LRESULT index = ::SendMessageW(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
		::SendMessageW(nppData._nppHandle, NPPM_ACTIVATEDOC, index >> 30, index & 0x3FFFFFFF);
	}
}


std::pair<intptr_t, intptr_t> getSelectionLines(int view)
{
	if (isSelectionVertical(view) || isMultiSelection(view))
		return std::make_pair(-1, -1);

	const intptr_t selectionStart = CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0);
	const intptr_t selectionEnd = CallScintilla(view, SCI_GETSELECTIONEND, 0, 0);

	if (selectionEnd - selectionStart == 0)
		return std::make_pair(-1, -1);

	intptr_t startLine	= CallScintilla(view, SCI_LINEFROMPOSITION, selectionStart, 0);
	intptr_t endLine	= CallScintilla(view, SCI_LINEFROMPOSITION, selectionEnd, 0);

	if (selectionEnd == getLineStart(view, endLine))
		--endLine;

	return std::make_pair(startLine, endLine);
}


int showArrowSymbol(int view, intptr_t line, bool down)
{
	const bool isRTL = isRTLwindow(getView(view));
	const unsigned char* rgba = down ?
			(isRTL ? icon_arrow_down_rtl : icon_arrow_down) : (isRTL ? icon_arrow_up_rtl : icon_arrow_up);

	CallScintilla(view,	SCI_MARKERDEFINERGBAIMAGE,	MARKER_ARROW_SYMBOL, (LPARAM)rgba);

	return static_cast<int>(CallScintilla(view, SCI_MARKERADD, line, MARKER_ARROW_SYMBOL));
}


void blinkLine(int view, intptr_t line)
{
	const int marker = CallScintilla(view, SCI_MARKERGET, line, 0) & MARKER_MASK_ALL;
	HWND hView = getView(view);

	for (int i = cBlinkCount; ;)
	{
		if (marker)
			clearMarks(view, line);
		else
			CallScintilla(view, SCI_MARKERADDSET, line, MARKER_MASK_BLANK);

		::UpdateWindow(hView);
		::Sleep(cBlinkInterval_ms);

		if (marker)
			CallScintilla(view, SCI_MARKERADDSET, line, marker);
		else
			CallScintilla(view, SCI_MARKERDELETE, line, MARKER_BLANK);

		::UpdateWindow(hView);

		if (--i == 0)
			break;

		::Sleep(cBlinkInterval_ms);
	}
}


void blinkRange(int view, intptr_t startPos, intptr_t endPos)
{
	ViewLocation loc(view);
	const std::pair<intptr_t, intptr_t> sel = getSelection(view);

	for (int i = cBlinkCount; ;)
	{
		setSelection(view, startPos, endPos, true);
		::UpdateWindow(getView(view));
		::Sleep(cBlinkInterval_ms);

		if (--i == 0)
			break;

		setSelection(view, startPos, startPos);
		::UpdateWindow(getView(view));
		::Sleep(cBlinkInterval_ms);
	}

	setSelection(view, sel.first, sel.second);
	loc.restore();
}


void centerAt(int view, intptr_t line)
{
	const intptr_t firstVisible = getVisibleFromDocLine(view, line) - CallScintilla(view, SCI_LINESONSCREEN, 0, 0) / 2;

	CallScintilla(view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
}


void setNormalView(int view)
{
	if (compareMode[view])
	{
		compareMode[view] = false;

		CallScintilla(view, SCI_SETENDATLASTLINE, endAtLastLine[view], 0);

		if (marginNum >= 0)
		{
			CallScintilla(view, SCI_SETMARGINMASKN, marginNum, 0);
			CallScintilla(view, SCI_SETMARGINWIDTHN, marginNum, 0);
			CallScintilla(view, SCI_SETMARGINSENSITIVEN, marginNum, false);
		}

		if (CallScintilla(view, SCI_GETCARETLINEVISIBLE, 0, 0))
		{
			CallScintilla(view, SCI_SETCARETLINEVISIBLE, false, 0);
			CallScintilla(view, SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, caretLineColor[view]);
			CallScintilla(view, SCI_SETCARETLINELAYER, caretLineLayer[view], 0);
			CallScintilla(view, SCI_SETCARETLINEVISIBLE, true, 0);
		}

		CallScintilla(view, SCI_ANNOTATIONSETSTYLEOFFSET, 0, 0);
	}
}


void setCompareView(int view, bool showMargin, int blankColor, int caretLineTransp)
{
	if (!compareMode[view])
	{
		compareMode[view] = true;

		endAtLastLine[view] = (CallScintilla(view, SCI_GETENDATLASTLINE, 0, 0) != 0);
		CallScintilla(view, SCI_SETENDATLASTLINE, false, 0);

		if (showMargin && marginNum >= 0)
		{
			CallScintilla(view, SCI_SETMARGINMASKN, marginNum, (LPARAM)(MARKER_MASK_SYMBOL | MARKER_MASK_ARROW));
			CallScintilla(view, SCI_SETMARGINWIDTHN, marginNum, gMarginWidth);
			CallScintilla(view, SCI_SETMARGINSENSITIVEN, marginNum, true);
		}

		caretLineColor[view] =
				static_cast<int>(CallScintilla(view, SCI_GETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, 0));
		caretLineLayer[view] = static_cast<int>(CallScintilla(view, SCI_GETCARETLINELAYER, 0, 0));
	}

	if (CallScintilla(view, SCI_GETCARETLINEVISIBLE, 0, 0))
	{
		const intptr_t alpha = ((100 - caretLineTransp) * SC_ALPHA_OPAQUE / 100);

		CallScintilla(view, SCI_SETCARETLINEVISIBLE, false, 0);
		CallScintilla(view, SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK,
				(caretLineColor[view] & 0xFFFFFF) | (alpha << 24));
		CallScintilla(view, SCI_SETCARETLINELAYER, SC_LAYER_UNDER_TEXT, 0);
		CallScintilla(view, SCI_SETCARETLINEVISIBLE, true, 0);
	}

	// For some reason the annotation blank styling is lost on Sci doc switch thus we need to reapply it
	setBlanksStyle(view, blankColor);
}


bool isDarkMode()
{
	if (::SendMessageW(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0))
		return true;

	const int bg = static_cast<int>(::SendMessageW(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0));

	const int r = bg & 0xFF;
	const int g = bg >> 8 & 0xFF;
	const int b = bg >> 16 & 0xFF;

	return (((r + g + b) / 3) < 128);
}


std::unique_ptr<NppDarkMode::Colors> getNppDarkModeColors()
{
	std::unique_ptr<NppDarkMode::Colors> dmColors = std::make_unique<NppDarkMode::Colors>();

	if (!::SendMessageW(nppData._nppHandle, NPPM_GETDARKMODECOLORS,
			sizeof(NppDarkMode::Colors), reinterpret_cast<LPARAM>(dmColors.get())))
		return nullptr;

	return dmColors;
}


std::vector<std::wstring> getOpenedFiles()
{
	const int mainViewFilesCount =
			static_cast<int>(::SendMessageW(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, PRIMARY_VIEW));
	const int subViewFilesCount =
			static_cast<int>(::SendMessageW(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, SECOND_VIEW));
	const int openedFilesCount = mainViewFilesCount + subViewFilesCount;

	std::vector<std::wstring> openedFiles(openedFilesCount);

	int i = 0;
	for (; i < mainViewFilesCount; ++i)
	{
		const LRESULT buffId = ::SendMessageW(nppData._nppHandle, NPPM_GETBUFFERIDFROMPOS, i, MAIN_VIEW);
		const LRESULT len = ::SendMessageW(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffId, (LPARAM)nullptr);
		openedFiles[i].resize(len);
		::SendMessageW(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffId, (LPARAM)openedFiles[i].data());
	}

	for (int j = 0; j < subViewFilesCount; ++j)
	{
		const LRESULT buffId = ::SendMessageW(nppData._nppHandle, NPPM_GETBUFFERIDFROMPOS, j, SUB_VIEW);
		const LRESULT len = ::SendMessageW(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffId, (LPARAM)nullptr);
		openedFiles[i].resize(len);
		::SendMessageW(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffId, (LPARAM)openedFiles[i].data());
		++i;
	}

	return openedFiles;
}


void setStyles(UserSettings& settings)
{
	const int bg = static_cast<int>(::SendMessageW(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0));

	settings.colors()._default = bg;

	int r = bg & 0xFF;
	int g = bg >> 8 & 0xFF;
	int b = bg >> 16 & 0xFF;

	constexpr int colorShift = 20;

	r = (r > colorShift) ? (r - colorShift) & 0xFF : 0;
	g = (g > colorShift) ? (g - colorShift) & 0xFF : 0;
	b = (b > colorShift) ? (b - colorShift) & 0xFF : 0;

	settings.colors().blank = r | (g << 8) | (b << 16);

	defineColor(MARKER_ADDED_LINE,		settings.colors().added);
	defineColor(MARKER_REMOVED_LINE,	settings.colors().removed);
	defineColor(MARKER_MOVED_LINE,		settings.colors().moved);
	defineColor(MARKER_CHANGED_LINE,	settings.colors().changed);
	defineColor(MARKER_BLANK,			settings.colors().blank);

	// Currently all icons are 14x14
	CallScintilla(MAIN_VIEW, SCI_RGBAIMAGESETWIDTH, 14, 0);
	CallScintilla(MAIN_VIEW, SCI_RGBAIMAGESETHEIGHT, 14, 0);

	CallScintilla(SUB_VIEW, SCI_RGBAIMAGESETWIDTH, 14, 0);
	CallScintilla(SUB_VIEW, SCI_RGBAIMAGESETHEIGHT, 14, 0);

	defineRgbaSymbol(MARKER_CHANGED_SYMBOL,				icon_changed);
	defineRgbaSymbol(MARKER_CHANGED_LOCAL_SYMBOL,		icon_changed_local);
	defineRgbaSymbol(MARKER_ADDED_SYMBOL,				icon_added);
	defineRgbaSymbol(MARKER_ADDED_LOCAL_SYMBOL,			icon_added_local);
	defineRgbaSymbol(MARKER_REMOVED_SYMBOL,				icon_removed);
	defineRgbaSymbol(MARKER_REMOVED_LOCAL_SYMBOL,		icon_removed_local);
	defineRgbaSymbol(MARKER_MOVED_LINE_SYMBOL,			icon_moved_line);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_BEGIN_SYMBOL,	icon_moved_block_start);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_MID_SYMBOL,		icon_moved_block_middle);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_END_SYMBOL,		icon_moved_block_end);

	setTextStyle(settings.colors().part_transparency);
}


bool isCurrentFileSaved()
{
	HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	MENUITEMINFOW mi { 0 };
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STATE;

	::GetMenuItemInfoW(hMenu, IDM_FILE_SAVE, FALSE, &mi);

	return (mi.fState & MFS_DISABLED);
}


void markTextAsChanged(int view, intptr_t start, intptr_t length, int color)
{
	if (length > 0)
	{
		const int curIndic = static_cast<int>(CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0));
		CallScintilla(view, SCI_SETINDICATORCURRENT, indicatorHighlight, 0);
		CallScintilla(view, SCI_SETINDICATORVALUE, color | SC_INDICVALUEBIT, 0);
		CallScintilla(view, SCI_INDICATORFILLRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void clearChangedIndicator(int view, intptr_t start, intptr_t length)
{
	if (length > 0)
	{
		const int curIndic = static_cast<int>(CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0));
		CallScintilla(view, SCI_SETINDICATORCURRENT, indicatorHighlight, 0);
		CallScintilla(view, SCI_INDICATORCLEARRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


std::vector<char> getText(int view, intptr_t startPos, intptr_t endPos)
{
	const intptr_t len = endPos - startPos;

	if (len <= 0)
		return {};

	std::vector<char> text(len + 1, 0);

	Sci_TextRangeFull tr;
	tr.chrg.cpMin = startPos;
	tr.chrg.cpMax = endPos;
	tr.lpstrText = text.data();

	CallScintilla(view, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);

	return text;
}


void clearWindow(int view, bool clearIndicators)
{
	CallScintilla(view, SCI_ANNOTATIONCLEARALL, 0, 0);

	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LOCAL_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_LOCAL_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LOCAL_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_BEGIN_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_MID_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_END_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);

	if (clearIndicators)
		clearChangedIndicatorFull(view);

	CallScintilla(view, SCI_COLOURISE, 0, -1);
}


void clearMarks(int view, intptr_t startLine, intptr_t length)
{
	intptr_t linesCount = getLinesCount(view);

	if (startLine + length < linesCount)
		linesCount = startLine + length;

	const intptr_t startPos = getLineStart(view, startLine);

	clearChangedIndicator(view, startPos, getLineEnd(view, linesCount - 1) - startPos);

	for (; startLine < linesCount; ++startLine)
		clearMarks(view, startLine);
}


intptr_t getPrevUnmarkedLine(int view, intptr_t startLine, int markMask)
{
	for (; (startLine >= 0) && isLineMarked(view, startLine, markMask); --startLine);

	return startLine;
}


intptr_t getNextUnmarkedLine(int view, intptr_t startLine, int markMask)
{
	const intptr_t linesCount = getLinesCount(view);

	for (; (startLine < linesCount) && isLineMarked(view, startLine, markMask); ++startLine);

	return ((startLine < linesCount) ? startLine : -1);
}


std::pair<intptr_t, intptr_t> getMarkedSection(int view, intptr_t startLine, intptr_t endLine, int markMask,
	bool excludeNewLine)
{
	const intptr_t lastLine = getEndLine(view);

	if ((startLine < 0) || (endLine > lastLine) || (startLine > endLine) || !isLineMarked(view, startLine, markMask))
		return std::make_pair(-1, -1);

	if ((startLine != endLine) && (!isLineMarked(view, endLine, markMask)))
		return std::make_pair(-1, -1);

	const intptr_t line1	= getPrevUnmarkedLine(view, startLine, markMask) + 1;
	intptr_t line2			= getNextUnmarkedLine(view, endLine, markMask);

	if (excludeNewLine)
		--line2;

	const intptr_t endPos = (line2 < 0) ? getLineEnd(view, lastLine) :
			(excludeNewLine ? getLineEnd(view, line2) : getLineStart(view, line2));

	return std::make_pair(getLineStart(view, line1), endPos);
}


std::vector<int> getMarkers(int view, intptr_t startLine, intptr_t length, int markMask, bool clearMarkers)
{
	std::vector<int> markers;

	if (length <= 0 || startLine < 0)
		return markers;

	const intptr_t linesCount = getLinesCount(view);

	if (startLine + length > linesCount)
		length = linesCount - startLine;

	if (clearMarkers)
	{
		const intptr_t startPos = getLineStart(view, startLine);
		clearChangedIndicator(view, startPos, getLineEnd(view, startLine + length - 1) - startPos);
	}

	markers.resize(length, 0);

	for (intptr_t line = CallScintilla(view, SCI_MARKERPREVIOUS, startLine + length - 1, markMask); line >= startLine;
		line = CallScintilla(view, SCI_MARKERPREVIOUS, line - 1, markMask))
	{
		markers[line - startLine] = CallScintilla(view, SCI_MARKERGET, line, 0) & markMask;

		if (clearMarkers)
			clearMarks(view, line);
	}

	return markers;
}


void setMarkers(int view, intptr_t startLine, const std::vector<int> &markers)
{
	const intptr_t linesCount = static_cast<intptr_t>(markers.size());

	if (startLine < 0 || linesCount == 0)
		return;

	const intptr_t startPos = getLineStart(view, startLine);
	clearChangedIndicator(view, startPos, getLineEnd(view, startLine + linesCount - 1) - startPos);

	for (intptr_t i = 0; i < linesCount; ++i)
	{
		clearMarks(view, startLine + i);

		if (markers[i])
			CallScintilla(view, SCI_MARKERADDSET, startLine + i, markers[i]);
	}
}


void unhideAllLines(int view)
{
	const intptr_t linesCount = getLinesCount(view);

	auto foldedLines = getFoldedLines(view);
	CallScintilla(view, SCI_SHOWLINES, 0, linesCount - 1);
	setFoldedLines(view, foldedLines);
}


void unhideLinesInRange(int view, intptr_t line, intptr_t length)
{
	if (line >= 0 && length > 0)
	{
		const intptr_t linesCount = getLinesCount(view);

		if (line + length > linesCount)
			length = linesCount - line;

		CallScintilla(view, SCI_SHOWLINES, line, line + length - 1);
	}
}


void hideLinesOutsideRange(int view, intptr_t startLine, intptr_t endLine)
{
	if (endLine <= startLine)
		return;

	const intptr_t linesCount = getLinesCount(view);

	if (startLine >= 0 && endLine < linesCount)
	{
		auto foldedLines = getFoldedLines(view);
		CallScintilla(view, SCI_SHOWLINES, startLine, endLine);
		setFoldedLines(view, foldedLines);
	}

	// First line (0) cannot be hidden so start from line 1
	if (startLine > 1)
		CallScintilla(view, SCI_HIDELINES, 1, startLine - 1);

	if (endLine > 0 && endLine + 1 < linesCount)
		CallScintilla(view, SCI_HIDELINES, endLine + 1, linesCount - 1);
}


void hideLines(int view, int hideMarkMask, bool hideUnmarked)
{
	const intptr_t linesCount = getLinesCount(view);
	const int otherMarkMask = (MARKER_MASK_LINE ^ hideMarkMask) & MARKER_MASK_LINE;

	// First line (0) cannot be hidden so start from line 1
	for (intptr_t endLine, startLine = 1; startLine < linesCount; startLine = endLine)
	{
		if (hideUnmarked)
		{
			for (; (startLine < linesCount) && isLineMarked(view, startLine, otherMarkMask); ++startLine);

			if (startLine == linesCount)
				break;

			endLine = CallScintilla(view, SCI_MARKERNEXT, startLine, otherMarkMask);

			if (endLine < 0)
				endLine = linesCount;
		}
		else
		{
			for (; (startLine < linesCount) && !isLineMarked(view, startLine, hideMarkMask); ++startLine);

			if (startLine == linesCount)
				break;

			for (endLine = startLine + 1;
				(endLine < linesCount) && isLineMarked(view, endLine, hideMarkMask); ++endLine);
		}

		CallScintilla(view, SCI_HIDELINES, startLine, endLine - 1);
	}
}


void hideNotepadHiddenLines(int view)
{
	constexpr int nppMarkHiddenBegin	= 1 << MARK_HIDELINESBEGIN;
	constexpr int nppMarkHiddenEnd		= 1 << MARK_HIDELINESEND;

	intptr_t endLine = 0;

	for (intptr_t startLine = CallScintilla(view, SCI_MARKERNEXT, 0, nppMarkHiddenBegin); startLine > 0;
		startLine = CallScintilla(view, SCI_MARKERNEXT, endLine, nppMarkHiddenBegin))
	{
		endLine = CallScintilla(view, SCI_MARKERNEXT, startLine, nppMarkHiddenEnd);

		if (endLine <= 0)
			break;

		CallScintilla(view, SCI_HIDELINES, startLine + 1, endLine - 1);
	}
}


bool isAdjacentAnnotation(int view, intptr_t line, bool down)
{
	if (down)
	{
		if (isLineAnnotated(view, line))
			return true;
	}
	else
	{
		if (line && isLineAnnotated(view, getPreviousUnhiddenLine(view, line)))
			return true;
	}

	return false;
}


bool isAdjacentAnnotationVisible(int view, intptr_t line, bool down)
{
	if (down)
	{
		if (!isLineAnnotated(view, line))
			return false;

		if (getVisibleFromDocLine(view, line) + getWrapCount(view, line) > getLastVisibleLine(view))
			return false;
	}
	else
	{
		if (!line || !isLineAnnotated(view, getPreviousUnhiddenLine(view, line)))
			return false;

		if (getVisibleFromDocLine(view, line) - 1 < getFirstVisibleLine(view))
			return false;
	}

	return true;
}


void clearAnnotations(int view, intptr_t startLine, intptr_t length)
{
	intptr_t endLine = getLinesCount(view);

	if (startLine + length < endLine)
		endLine = startLine + length;

	for (; startLine < endLine; ++startLine)
		clearAnnotation(view, startLine);
}


std::vector<char> getLineText(int view, intptr_t line, bool includeEOL)
{
	const intptr_t startPos	= getLineStart(view, line);
	const intptr_t endPos	= includeEOL ?
			startPos + CallScintilla(view, SCI_LINELENGTH, line, 0) : getLineEnd(view, line);
	const int len			= static_cast<int>(endPos - startPos);

	if (len <= 0)
		return {};

	std::vector<char> text(len + 1, 0);

	Sci_TextRangeFull tr;
	tr.chrg.cpMin = startPos;
	tr.chrg.cpMax = endPos;
	tr.lpstrText = text.data();

	CallScintilla(view, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&tr);

	text.pop_back();

	return text;
}


intptr_t replaceText(int view, const std::string& txtToReplace, const std::string& replacementTxt,
	intptr_t searchStartLine)
{
	intptr_t pos = getLineStart(view, searchStartLine);

	CallScintilla(view, SCI_SETTARGETSTART, pos, 0);

	if (txtToReplace.empty())
	{
		CallScintilla(view, SCI_SETTARGETEND, pos, 0);
	}
	else
	{
		CallScintilla(view, SCI_SETTARGETEND, CallScintilla(view, SCI_GETLENGTH, 0, 0), 0);
		CallScintilla(view, SCI_SETSEARCHFLAGS, (WPARAM)SCFIND_MATCHCASE, 0);
		pos = CallScintilla(view, SCI_SEARCHINTARGET, txtToReplace.size(), (LPARAM)txtToReplace.c_str());
	}

	if (pos >= 0)
	{
		CallScintilla(view, SCI_REPLACETARGETMINIMAL, (uptr_t)-1, (LPARAM)replacementTxt.c_str());
		pos = CallScintilla(view, SCI_GETTARGETEND, 0, 0);
		pos = CallScintilla(view, SCI_LINEFROMPOSITION, pos, 0) + 1;
	}

	return pos;
}


void addBlankSection(int view, intptr_t line, intptr_t length, intptr_t textLinePos, const char *text)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');

	if (textLinePos > 0 && text != nullptr)
	{
		if (length < textLinePos)
			return;

		blank.insert(blank.begin() + textLinePos - 1, text, text + strlen(text));
	}

	blank.push_back('\0');

	CallScintilla(view, SCI_ANNOTATIONSETTEXT, getPreviousUnhiddenLine(view, line), (LPARAM)blank.data());
}


void addBlankSectionAfter(int view, intptr_t line, intptr_t length)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');
	blank.push_back('\0');

	CallScintilla(view, SCI_ANNOTATIONSETTEXT, getUnhiddenLine(view, line), (LPARAM)blank.data());
}


std::vector<intptr_t> getFoldedLines(int view)
{
	if (CallScintilla(view, SCI_GETALLLINESVISIBLE, 0, 0))
		return {};

	const intptr_t linesCount = getLinesCount(view);

	std::vector<intptr_t> foldedLines;

	for (intptr_t line = 0; line < linesCount; ++line)
	{
		line = CallScintilla(view, SCI_CONTRACTEDFOLDNEXT, line, 0);

		if (line < 0)
			break;

		foldedLines.emplace_back(line);
	}

	return foldedLines;
}


void setFoldedLines(int view, const std::vector<intptr_t>& foldedLines)
{
	for (auto line : foldedLines)
		CallScintilla(view, SCI_FOLDLINE, line, SC_FOLDACTION_CONTRACT);
}


void moveFileToOtherView()
{
	const int view = getCurrentViewId();

	auto foldedLines = getFoldedLines(view);

	::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_GOTO_ANOTHER_VIEW);

	setFoldedLines(getOtherViewId(view), foldedLines);
}


std::vector<uint8_t> generateContentsSha256(int view, intptr_t startLine, intptr_t endLine)
{
	if (startLine < 0)
		startLine = 0;

	if (endLine < 0)
		endLine = getEndLine(view);

	const auto selStart	= getLineStart(view, startLine);
	const auto selEnd	= getLineEnd(view, endLine);

	if (selStart == selEnd)
		return {};

	auto text = getText(view, selStart, selEnd);
	text.pop_back();

	return SHA256()(text);
}
