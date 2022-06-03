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


#include "Printer.h"
#include "RunDlg.h"
//#include "Parameters.h"

void replaceStr(generic_string & str, generic_string str2BeReplaced, generic_string replacement)
{
	size_t pos = str.find(str2BeReplaced);

	if (pos != str.npos)
		str.replace(pos, str2BeReplaced.length(), replacement);
}

void Printer::init(HINSTANCE hInst, HWND hwnd, ScintillaEditView *pSEView, bool showDialog, size_t startPos, size_t endPos, bool isRTL)
{
	_pSEView = pSEView;
	_startPos = startPos;
	_endPos = endPos;
	_pdlg.lStructSize = sizeof(PRINTDLG);
	_pdlg.hwndOwner = hwnd;
	_pdlg.hInstance = hInst;
	_pdlg.Flags = PD_USEDEVMODECOPIES | PD_ALLPAGES | PD_RETURNDC;
	_pdlg.nFromPage = 1;
	_pdlg.nToPage = 1;
	_pdlg.nMinPage = 1;
	_pdlg.nMaxPage = 0xffffU; // We do not know how many pages in the
							// document until the printer is selected and the paper size is known.
	_pdlg.nCopies = 1;
	_pdlg.hDC = 0;
	_pdlg.hDevMode = NULL;
	_pdlg.hDevNames = NULL;
	_pdlg.lCustData = 0;
	_pdlg.lpfnPrintHook = NULL;
	_pdlg.lpfnSetupHook = NULL;
	_pdlg.lpPrintTemplateName = NULL;
	_pdlg.lpSetupTemplateName = NULL;
	_pdlg.hPrintTemplate = NULL;
	_pdlg.hSetupTemplate = NULL;

	// See if a range has been selected
	_pdlg.Flags |= (_startPos != _endPos)?PD_SELECTION:PD_NOSELECTION;

	if (!showDialog) 
	{
		// Don't display dialog box, just use the default printer and options
		_pdlg.Flags |= PD_RETURNDEFAULT;
	}
	_isRTL = isRTL;
}



size_t Printer::doPrint(bool justDoIt)
{
	const NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();

	POINT ptPage;
	POINT ptDpi;

	RECT rectMargins;
	RECT rectPhysMargins;
	RECT userMargins;

	// Get printer resolution
	ptDpi.x = GetDeviceCaps(_pdlg.hDC, LOGPIXELSX);    // dpi in X direction
	ptDpi.y = GetDeviceCaps(_pdlg.hDC, LOGPIXELSY);    // dpi in Y direction

	// Start by getting the physical page size (in device units).
	ptPage.x = GetDeviceCaps(_pdlg.hDC, PHYSICALWIDTH);   // device units
	ptPage.y = GetDeviceCaps(_pdlg.hDC, PHYSICALHEIGHT);  // device units

	// Get the dimensions of the unprintable
	// part of the page (in device units).
	rectPhysMargins.left = GetDeviceCaps(_pdlg.hDC, PHYSICALOFFSETX);
	rectPhysMargins.top = GetDeviceCaps(_pdlg.hDC, PHYSICALOFFSETY);

	// To get the right and lower unprintable area,
	// we take the entire width and height of the paper and
	// subtract everything else.
	rectPhysMargins.right = ptPage.x						// total paper width
	                        - GetDeviceCaps(_pdlg.hDC, HORZRES) // printable width
	                        - rectPhysMargins.left;				// left unprintable margin

	rectPhysMargins.bottom = ptPage.y						// total paper height
	                         - GetDeviceCaps(_pdlg.hDC, VERTRES)	// printable height
	                         - rectPhysMargins.top;				// right unprintable margin
	if (nppGUI._printSettings.isUserMargePresent())
	{
		userMargins.left  = MulDiv(nppGUI._printSettings._marge.left*100, ptDpi.x, 2540);
		userMargins.top  = MulDiv(nppGUI._printSettings._marge.top*100, ptDpi.y, 2540);
		userMargins.right  = MulDiv(nppGUI._printSettings._marge.right*100, ptDpi.x, 2540);
		userMargins.bottom  = MulDiv(nppGUI._printSettings._marge.bottom*100, ptDpi.y, 2540);
	
		rectMargins.left	= max(rectPhysMargins.left, userMargins.left);
		rectMargins.top		= max(rectPhysMargins.top, userMargins.top);
		rectMargins.right	= max(rectPhysMargins.right, userMargins.right);
		rectMargins.bottom	= max(rectPhysMargins.bottom, userMargins.bottom);
	}
	else
	{
	rectMargins.left	= rectPhysMargins.left;
	rectMargins.top		= rectPhysMargins.top;
	rectMargins.right	= rectPhysMargins.right;
	rectMargins.bottom	= rectPhysMargins.bottom;
	}
	// Convert device coordinates into logical coordinates
	DPtoLP(_pdlg.hDC, (LPPOINT)&rectMargins, 2);
	DPtoLP(_pdlg.hDC, (LPPOINT)&rectPhysMargins, 2);

	// Convert page size to logical units and we're done!
	DPtoLP(_pdlg.hDC, &ptPage, 1);

	TEXTMETRIC tm;

	int fontSize = nppGUI._printSettings._headerFontSize?nppGUI._printSettings._headerFontSize:9;
	int fontWeight = (nppGUI._printSettings._headerFontStyle & FONTSTYLE_BOLD) ? FW_BOLD : FW_NORMAL;
	int isFontItalic = (nppGUI._printSettings._headerFontStyle & FONTSTYLE_ITALIC) ? TRUE : FALSE;
	const TCHAR *fontFace = (nppGUI._printSettings._headerFontName != TEXT(""))?nppGUI._printSettings._headerFontName.c_str():TEXT("Arial");

	int headerLineHeight = ::MulDiv(fontSize, ptDpi.y, 72);

	HFONT fontHeader = ::CreateFont(headerLineHeight,
	                                0, 0, 0,
	                                fontWeight,
	                                isFontItalic,
	                                FALSE,
	                                0, 0, 0,
	                                0, 0, 0,
	                                fontFace);

	::SelectObject(_pdlg.hDC, fontHeader);
	::GetTextMetrics(_pdlg.hDC, &tm);
	headerLineHeight = tm.tmHeight + tm.tmExternalLeading;

	fontSize = nppGUI._printSettings._footerFontSize?nppGUI._printSettings._footerFontSize:9;
	fontWeight = (nppGUI._printSettings._footerFontStyle & FONTSTYLE_BOLD) ? FW_BOLD : FW_NORMAL;
	isFontItalic = (nppGUI._printSettings._footerFontStyle & FONTSTYLE_ITALIC) ? TRUE : FALSE;
	fontFace = (nppGUI._printSettings._footerFontName != TEXT(""))?nppGUI._printSettings._footerFontName.c_str():TEXT("Arial");

	int footerLineHeight = ::MulDiv(fontSize, ptDpi.y, 72);
	HFONT fontFooter = ::CreateFont(footerLineHeight,
	                                0, 0, 0,
	                                fontWeight,
	                                isFontItalic,
	                                FALSE,
	                                0, 0, 0,
	                                0, 0, 0,
	                                fontFace);

	::SelectObject(_pdlg.hDC, fontFooter);
	::GetTextMetrics(_pdlg.hDC, &tm);
	footerLineHeight = tm.tmHeight + tm.tmExternalLeading;
	

	::GetTextMetrics(_pdlg.hDC, &tm);
	int printMarge = tm.tmHeight + tm.tmExternalLeading;
	printMarge = printMarge + printMarge / 2;

	DOCINFO docInfo;
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.fwType = 0;
	docInfo.lpszDocName = _pSEView->getCurrentBuffer()->getFullPathName();
	docInfo.lpszOutput = NULL;
	docInfo.lpszDatatype = NULL;

	if (::StartDoc(_pdlg.hDC, &docInfo) < 0) 
	{
		MessageBox(NULL, TEXT("Can not start printer document."), 0, MB_OK);
		return 0;
	}
	
	// By default, we will print all the document
	size_t lengthPrinted = 0;
	size_t lengthDoc = _pSEView->getCurrentDocLen();
	size_t lengthDocMax = lengthDoc;

	// In the case that the print dialog was launched and that there's a range of selection
	// We print the range of selection
	if ((!(_pdlg.Flags & PD_RETURNDEFAULT)) && (_pdlg.Flags & PD_SELECTION))
	{
		if (_startPos > _endPos) 
		{
			lengthPrinted = _endPos;
			lengthDoc = _startPos;
		}
		else 
		{
			lengthPrinted = _startPos;
			lengthDoc = _endPos;
		}

		if (lengthPrinted < 0)
			lengthPrinted = 0;
		if (lengthDoc > lengthDocMax)
			lengthDoc = lengthDocMax;
	}

	NPP_RangeToFormat frPrint;
	frPrint.hdc = _pdlg.hDC;
	frPrint.hdcTarget = _pdlg.hDC;
	frPrint.rc.left = rectMargins.left - rectPhysMargins.left;
	frPrint.rc.top = rectMargins.top - rectPhysMargins.top;
	frPrint.rc.right = ptPage.x - rectMargins.right - rectPhysMargins.left;
	frPrint.rc.bottom = ptPage.y - rectMargins.bottom - rectPhysMargins.top;
	frPrint.rcPage.left = 0;
	frPrint.rcPage.top = 0;
	frPrint.rcPage.right = ptPage.x - rectPhysMargins.left - rectPhysMargins.right - 1;
	frPrint.rcPage.bottom = ptPage.y - rectPhysMargins.top - rectPhysMargins.bottom - 1;

	frPrint.rc.top += printMarge;
	frPrint.rc.bottom -= printMarge;
	frPrint.rc.left += printMarge;
	frPrint.rc.right -= printMarge;

	const int headerSize = 256;
	TCHAR headerL[headerSize] = TEXT("");
	TCHAR headerM[headerSize] = TEXT("");
	TCHAR headerR[headerSize] = TEXT("");
	TCHAR footerL[headerSize] = TEXT("");
	TCHAR footerM[headerSize] = TEXT("");
	TCHAR footerR[headerSize] = TEXT("");
	

	const TCHAR shortDateVar[] = TEXT("$(SHORT_DATE)");
	const TCHAR longDateVar[] = TEXT("$(LONG_DATE)");
	const TCHAR timeVar[] = TEXT("$(TIME)");

	const int bufferSize = 64;
	TCHAR shortDate[bufferSize];
	TCHAR longDate[bufferSize];
	TCHAR time[bufferSize];

	SYSTEMTIME st;
	::GetLocalTime(&st);
	::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, shortDate, bufferSize);
	::GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, longDate, bufferSize);
	::GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, time, bufferSize);

	if (nppGUI._printSettings.isHeaderPresent())
	{
		frPrint.rc.top += headerLineHeight + headerLineHeight / 2;

		generic_string headerLeftPart = nppGUI._printSettings._headerLeft;
		if (headerLeftPart != TEXT(""))
		{
			replaceStr(headerLeftPart, shortDateVar, shortDate);
			replaceStr(headerLeftPart, longDateVar, longDate);
			replaceStr(headerLeftPart, timeVar, time);
			expandNppEnvironmentStrs(headerLeftPart.c_str(), headerL, headerSize, _pdlg.hwndOwner);
		}

		generic_string headerMiddlePart = nppGUI._printSettings._headerMiddle;
		if (headerMiddlePart != TEXT(""))
		{
			replaceStr(headerMiddlePart, shortDateVar, shortDate);
			replaceStr(headerMiddlePart, longDateVar, longDate);
			replaceStr(headerMiddlePart, timeVar, time);
			expandNppEnvironmentStrs(headerMiddlePart.c_str(), headerM, headerSize, _pdlg.hwndOwner);
		}

		generic_string headerRightPart = nppGUI._printSettings._headerRight;
		if (headerRightPart != TEXT(""))
		{
			replaceStr(headerRightPart, shortDateVar, shortDate);
			replaceStr(headerRightPart, longDateVar, longDate);
			replaceStr(headerRightPart, timeVar, time);
			expandNppEnvironmentStrs(headerRightPart.c_str(), headerR, headerSize, _pdlg.hwndOwner);
		}

	}

	if (nppGUI._printSettings.isFooterPresent())
	{
		frPrint.rc.bottom -= footerLineHeight + footerLineHeight / 2;

		generic_string footerLeftPart = nppGUI._printSettings._footerLeft;
		if (footerLeftPart != TEXT(""))
		{
			replaceStr(footerLeftPart, shortDateVar, shortDate);
			replaceStr(footerLeftPart, longDateVar, longDate);
			replaceStr(footerLeftPart, timeVar, time);
			expandNppEnvironmentStrs(footerLeftPart.c_str(), footerL, headerSize, _pdlg.hwndOwner);
		}

		generic_string footerMiddlePart = nppGUI._printSettings._footerMiddle;
		if (footerMiddlePart != TEXT(""))
		{
			replaceStr(footerMiddlePart, shortDateVar, shortDate);
			replaceStr(footerMiddlePart, longDateVar, longDate);
			replaceStr(footerMiddlePart, timeVar, time);
			expandNppEnvironmentStrs(footerMiddlePart.c_str(), footerM, headerSize, _pdlg.hwndOwner);
		}

		generic_string footerRightPart = nppGUI._printSettings._footerRight;
		if (footerRightPart != TEXT(""))
		{
			replaceStr(footerRightPart, shortDateVar, shortDate);
			replaceStr(footerRightPart, longDateVar, longDate);
			replaceStr(footerRightPart, timeVar, time);
			expandNppEnvironmentStrs(footerRightPart.c_str(), footerR, headerSize, _pdlg.hwndOwner);
		}
	}

	
	bool isShown = _pSEView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER);
	if (!nppGUI._printSettings._printLineNumber)
		_pSEView->showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, false);

	int pageNum = 1;
	const TCHAR pageVar[] = TEXT("$(CURRENT_PRINTING_PAGE)");

	_pSEView->execute(SCI_SETPRINTCOLOURMODE, nppGUI._printSettings._printOption); // setting mode once is enough
	while (lengthPrinted < lengthDoc) 
	{
		bool printPage = (!(_pdlg.Flags & PD_PAGENUMS) ||
		             (pageNum >= _pdlg.nFromPage) && (pageNum <= _pdlg.nToPage));
					 
		if (!justDoIt)
			printPage = false;		 

		TCHAR pageString[32];
		wsprintf(pageString, TEXT("%0d"), pageNum);
		
		if (printPage) 
		{
			::StartPage(_pdlg.hDC);

			if (nppGUI._printSettings.isHeaderPresent())
			{
				::SelectObject(_pdlg.hDC, fontHeader);

				::SetTextColor(_pdlg.hDC, RGB(0, 0, 0));
				::SetBkColor(_pdlg.hDC, RGB(255, 255, 255));

				UINT oldTASettings = ::SetTextAlign(_pdlg.hDC, _isRTL ? TA_RTLREADING | TA_BOTTOM : TA_BOTTOM);
				RECT rcw = {frPrint.rc.left, frPrint.rc.top - headerLineHeight - headerLineHeight / 2,
							frPrint.rc.right, frPrint.rc.top - headerLineHeight / 2};
				rcw.bottom = rcw.top + headerLineHeight;


				SIZE size;
				
				// Left part
				if (headerL[0] != '\0')
				{
					generic_string headerLeft(headerL);
					size_t pos = headerLeft.find(pageVar);

					if (pos != headerLeft.npos)
						headerLeft.replace(pos, lstrlen(pageVar), pageString);

					::ExtTextOut(_pdlg.hDC, frPrint.rc.left + 5, frPrint.rc.top - headerLineHeight / 2,
						ETO_OPAQUE, &rcw, headerLeft.c_str(), static_cast<int>(headerLeft.length()), NULL);
				}

				// Middle part
				if (headerM[0] != '\0')
				{
					generic_string headerMiddle(headerM);
					size_t pos = headerMiddle.find(pageVar);
					if (pos != headerMiddle.npos)
						headerMiddle.replace(pos, lstrlen(pageVar), pageString);

					::GetTextExtentPoint32(_pdlg.hDC, headerMiddle.c_str(), static_cast<int>(headerMiddle.length()), &size);
					::ExtTextOut(_pdlg.hDC, ((frPrint.rc.right - frPrint.rc.left)/2 + frPrint.rc.left) - (size.cx/2), frPrint.rc.top - headerLineHeight / 2,
						ETO_CLIPPED, &rcw, headerMiddle.c_str(), static_cast<int>(headerMiddle.length()), NULL);
				}
				// Right part
				if (headerR[0] != '\0')
				{
					generic_string headerRight(headerR);
					size_t pos = headerRight.find(pageVar);
					if (pos != headerRight.npos)
						headerRight.replace(pos, lstrlen(pageVar), pageString);

					::GetTextExtentPoint32(_pdlg.hDC, headerRight.c_str(), static_cast<int>(headerRight.length()), &size);
					::ExtTextOut(_pdlg.hDC, frPrint.rc.right - size.cx, frPrint.rc.top - headerLineHeight / 2,
						ETO_CLIPPED, &rcw, headerRight.c_str(), static_cast<int>(headerRight.length()), NULL);
				}

				::SetTextAlign(_pdlg.hDC, oldTASettings);
				HPEN pen = ::CreatePen(0, 1, 0x00000000);
				HPEN penOld = static_cast<HPEN>(::SelectObject(_pdlg.hDC, pen));
				::MoveToEx(_pdlg.hDC, frPrint.rc.left, frPrint.rc.top - headerLineHeight / 4, NULL);
				::LineTo(_pdlg.hDC, frPrint.rc.right, frPrint.rc.top - headerLineHeight / 4);
				::SelectObject(_pdlg.hDC, penOld);
				::DeleteObject(pen);
			}
		}
		
		frPrint.chrg.cpMin = static_cast<Sci_Position>(lengthPrinted);
		frPrint.chrg.cpMax = static_cast<Sci_Position>(lengthDoc);
		lengthPrinted = _pSEView->execute(SCI_FORMATRANGEFULL, printPage, reinterpret_cast<LPARAM>(&frPrint));

		if (printPage) 
		{
			if (nppGUI._printSettings.isFooterPresent())
			{
				::SelectObject(_pdlg.hDC, fontFooter);
				
				::SetTextColor(_pdlg.hDC, RGB(0, 0, 0));
				::SetBkColor(_pdlg.hDC, RGB(255, 255, 255));

				UINT oldta = ::SetTextAlign(_pdlg.hDC, _isRTL ? TA_RTLREADING | TA_TOP : TA_TOP);
				RECT rcw = {frPrint.rc.left, frPrint.rc.bottom + footerLineHeight / 2,
					        frPrint.rc.right, frPrint.rc.bottom + footerLineHeight + footerLineHeight / 2};

				SIZE size;
				
				// Left part
				if (footerL[0] != '\0')
				{
					generic_string footerLeft(footerL);
					size_t pos = footerLeft.find(pageVar);
					if (pos != footerLeft.npos)
						footerLeft.replace(pos, lstrlen(pageVar), pageString);

					::ExtTextOut(_pdlg.hDC, frPrint.rc.left + 5, frPrint.rc.bottom + footerLineHeight / 2,
						ETO_OPAQUE, &rcw, footerLeft.c_str(), static_cast<int>(footerLeft.length()), NULL);
				}

				// Middle part
				if (footerM[0] != '\0')
				{
					generic_string footerMiddle(footerM);
					size_t pos = footerMiddle.find(pageVar);
					if (pos != footerMiddle.npos)
						footerMiddle.replace(pos, lstrlen(pageVar), pageString);

					::GetTextExtentPoint32(_pdlg.hDC, footerMiddle.c_str(), static_cast<int>(footerMiddle.length()), &size);
					::ExtTextOut(_pdlg.hDC, ((frPrint.rc.right - frPrint.rc.left)/2 + frPrint.rc.left) - (size.cx/2), frPrint.rc.bottom + footerLineHeight / 2,
									ETO_CLIPPED, &rcw, footerMiddle.c_str(), static_cast<int>(footerMiddle.length()), NULL);
				}
				// Right part
				if (footerR[0] != '\0')
				{
					generic_string footerRight(footerR);
					size_t pos = footerRight.find(pageVar);
					if (pos != footerRight.npos)
						footerRight.replace(pos, lstrlen(pageVar), pageString);
					::GetTextExtentPoint32(_pdlg.hDC, footerRight.c_str(), static_cast<int>(footerRight.length()), &size);
					::ExtTextOut(_pdlg.hDC, frPrint.rc.right - size.cx, frPrint.rc.bottom + footerLineHeight / 2,
									ETO_CLIPPED, &rcw, footerRight.c_str(), static_cast<int>(footerRight.length()), NULL);
				}

				::SetTextAlign(_pdlg.hDC, oldta);
				HPEN pen = ::CreatePen(0, 1, 0x00000000);
				HPEN penOld = static_cast<HPEN>(::SelectObject(_pdlg.hDC, pen));

				::MoveToEx(_pdlg.hDC, frPrint.rc.left, frPrint.rc.bottom + footerLineHeight / 4, NULL);
				::LineTo(_pdlg.hDC, frPrint.rc.right, frPrint.rc.bottom + footerLineHeight / 4);
				::SelectObject(_pdlg.hDC, penOld);
				::DeleteObject(pen);
			}
			
			::EndPage(_pdlg.hDC);
		}

		++pageNum;

		if ((_pdlg.Flags & PD_PAGENUMS) && (pageNum > _pdlg.nToPage))
			break;
	}

	if (!nppGUI._printSettings._printLineNumber)
		_pSEView->showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, isShown);

	_pSEView->execute(SCI_FORMATRANGEFULL, FALSE, 0);
	::EndDoc(_pdlg.hDC);
	::DeleteDC(_pdlg.hDC);

	if (fontHeader)
		::DeleteObject(fontHeader);

	if (fontFooter)
		::DeleteObject(fontFooter);

	return (pageNum - 1);
}


