//this file is part of notepad++
//Copyright (C)2003 Harry <harrybharry@users.sourceforge.net>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "UrlHighlighter.h"
#include "Parameters.h"

#define MAXLINEHIGHLIGHT 400	//prevent highlighter from doing too much work when a lot is visible

const char *urlHttpRegExpr = "http://[a-z0-9_\\-\\+.:?&@=/%#]*";
const char *urlHttpsRegExpr ="https://[a-z0-9_\\-\\+.:?&@=/%#]*";
const char *urlFtpRegExpr = "ftp://[a-z0-9_\\-\\+.:?&@=/%#]*";
const char *emailRegExpr = "mailto:[a-z0-9._-]+@[a-z0-9.-]+\\.[a-z]+";

//Used to map style to style with hotspot properties
struct HotspotStyle {
	int styleID;
	int hotspotID;
	HotspotStyle(int sid, int hid) : styleID(sid), hotspotID(hid) {};
};

UrlHighlighter::UrlHighlighter(FindReplaceDlg * pFRDlg)
: _pFRDlg(pFRDlg), _isDrawing(false)
{
	//Nothing to do
}


void UrlHighlighter::highlightView(ScintillaEditView * pHighlightView)
{
	if (_isDrawing)
		return;
	_isDrawing = true;

	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	//Test if highlight is needed
	int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
	if ((urlAction != 1) && (urlAction != 2))
		return;

	// Save target locations for other search functions
	int loc = (int)pHighlightView->execute(SCI_GETENDSTYLED);
	int originalStartPos = (int)pHighlightView->execute(SCI_GETTARGETSTART);
	int originalEndPos = (int)pHighlightView->execute(SCI_GETTARGETEND);

	// Get the range of text visible and highlight everything in it
	int firstLine =		(int)pHighlightView->execute(SCI_GETFIRSTVISIBLELINE);
	int nrLines =	min((int)pHighlightView->execute(SCI_LINESONSCREEN), MAXLINEHIGHLIGHT ) + 1;
	int lastLine =		(int)pHighlightView->execute(SCI_DOCLINEFROMVISIBLE, firstLine + nrLines);
	const int startPos =(int)pHighlightView->execute(SCI_POSITIONFROMLINE, firstLine);
	int endPos =		(int)pHighlightView->execute(SCI_POSITIONFROMLINE, lastLine);
	if (endPos == -1)	//past EOF
		endPos =		(int)pHighlightView->getCurrentDocLen() - 1;


	int preEndStyled = (int)pHighlightView->execute(SCI_GETENDSTYLED);
	int nextHotspotStyle = 31;	//First style to be used for hotspot

	vector< HotspotStyle > hotspotStylers;	//current stylers that map normal style to style with hotspot

	pHighlightView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

	vector<const char *> urlRegExpressions;
	urlRegExpressions.push_back(urlHttpRegExpr);
	urlRegExpressions.push_back(urlHttpsRegExpr);
	urlRegExpressions.push_back(urlFtpRegExpr);
	urlRegExpressions.push_back(emailRegExpr);

	int hotSpotStyle = 31;

	pHighlightView->execute(SCI_STYLESETFORE,		hotSpotStyle, blue);
	pHighlightView->execute(SCI_STYLESETUNDERLINE,	hotSpotStyle, TRUE);
	pHighlightView->execute(SCI_STYLESETHOTSPOT,	hotSpotStyle, TRUE);

	for (size_t k = 0 ; k < urlRegExpressions.size() ; k++)
	{
		pHighlightView->execute(SCI_SETTARGETSTART, startPos);
		pHighlightView->execute(SCI_SETTARGETEND, endPos);

		int targetStart = 0;
		int targetEnd = 0;
		targetStart = pHighlightView->execute(SCI_SEARCHINTARGET, strlen(urlRegExpressions[k]), (LPARAM)urlRegExpressions[k]);
		while (targetStart != -1)
		{
			targetStart = (int)(pHighlightView->execute(SCI_GETTARGETSTART));
			targetEnd = (int)(pHighlightView->execute(SCI_GETTARGETEND));
			int foundTextLen = targetEnd - targetStart;
			int idStyle = pHighlightView->execute(SCI_GETSTYLEAT, targetStart);

			pHighlightView->execute(SCI_STARTSTYLING, targetStart, 0xFF);
			pHighlightView->execute(SCI_SETSTYLING, foundTextLen, hotSpotStyle);

 			int startMovingPos = targetStart + foundTextLen;

			if (startMovingPos >= endPos)
			{
				targetStart = -1;
			}
			else
			{
				pHighlightView->execute(SCI_SETTARGETSTART, startMovingPos);
				pHighlightView->execute(SCI_SETTARGETEND, endPos);
				targetStart = pHighlightView->execute(SCI_SEARCHINTARGET, strlen(urlRegExpressions[k]), (LPARAM)urlRegExpressions[k]);
			}
		}
	}
	pHighlightView->execute(SCI_STARTSTYLING, preEndStyled, 0xFF);
	
	// restore the original targets to avoid conflicts with the search/replace functions
	pHighlightView->execute(SCI_SETTARGETSTART, originalStartPos);
	pHighlightView->execute(SCI_SETTARGETEND, originalEndPos);
	_isDrawing = false;
}


/*
void UrlHighlighter::highlightView(ScintillaEditView * pHighlightView)
{
	if (_isDrawing)
		return;
	_isDrawing = true;

	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
	//Test if highlight is needed
	int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
	if ((urlAction != 1) && (urlAction != 2))
		return;

	// Save target locations for other search functions
	int loc = (int)pHighlightView->execute(SCI_GETENDSTYLED);
	int originalStartPos = (int)pHighlightView->execute(SCI_GETTARGETSTART);
	int originalEndPos = (int)pHighlightView->execute(SCI_GETTARGETEND);

	// Get the range of text visible and highlight everything in it
	int firstLine =		(int)pHighlightView->execute(SCI_GETFIRSTVISIBLELINE);
	int nrLines =	min((int)pHighlightView->execute(SCI_LINESONSCREEN), MAXLINEHIGHLIGHT ) + 1;
	int lastLine =		(int)pHighlightView->execute(SCI_DOCLINEFROMVISIBLE, firstLine + nrLines);
	const int startPos =(int)pHighlightView->execute(SCI_POSITIONFROMLINE, firstLine);
	int endPos =		(int)pHighlightView->execute(SCI_POSITIONFROMLINE, lastLine);
	if (endPos == -1)	//past EOF
		endPos =		(int)pHighlightView->getCurrentDocLen() - 1;


	int preEndStyled = (int)pHighlightView->execute(SCI_GETENDSTYLED);
	int nextHotspotStyle = 31;	//First style to be used for hotspot

	vector< HotspotStyle > hotspotStylers;	//current stylers that map normal style to style with hotspot

	pHighlightView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

	vector<const char *> urlRegExpressions;
	urlRegExpressions.push_back(urlHttpRegExpr);
	urlRegExpressions.push_back(urlHttpsRegExpr);
	urlRegExpressions.push_back(urlFtpRegExpr);
	urlRegExpressions.push_back(emailRegExpr);

	for (size_t k = 0 ; k < urlRegExpressions.size() ; k++)
	{
		pHighlightView->execute(SCI_SETTARGETSTART, startPos);
		pHighlightView->execute(SCI_SETTARGETEND, endPos);

		int targetStart = 0;
		int targetEnd = 0;
		targetStart = pHighlightView->execute(SCI_SEARCHINTARGET, strlen(urlRegExpressions[k]), (LPARAM)urlRegExpressions[k]);
		while (targetStart != -1)
		{
			targetStart = (int)(pHighlightView->execute(SCI_GETTARGETSTART));
			targetEnd = (int)(pHighlightView->execute(SCI_GETTARGETEND));
			int foundTextLen = targetEnd - targetStart;
			int idStyle = pHighlightView->execute(SCI_GETSTYLEAT, targetStart);

			int foundStyle = -1;
			for (size_t i = 0 ; i < hotspotStylers.size() ; i++)
			{
				if (hotspotStylers[i].styleID == idStyle)
				{
					foundStyle = hotspotStylers[i].hotspotID;
					break;
				}
				if (hotspotStylers[i].hotspotID == idStyle)
				{
					foundStyle = hotspotStylers[i].hotspotID;
					break;
				}
			}

			if (foundStyle == -1)	//no hotspot style for this style, create one
			{
				if (nextHotspotStyle == 0) 
				{
					//no style slots left
				}
				else 
				{
					hotspotStylers.push_back(HotspotStyle(idStyle, nextHotspotStyle));

					char fontName[256];
					Style hotspotStyle;

					hotspotStyle._styleID = nextHotspotStyle;
					pHighlightView->execute(SCI_STYLEGETFONT, idStyle, (LPARAM)fontName);
					hotspotStyle._fontName = &fontName[0];
					hotspotStyle._fgColor = pHighlightView->execute(SCI_STYLEGETFORE, idStyle);
					hotspotStyle._bgColor = pHighlightView->execute(SCI_STYLEGETBACK, idStyle);
					hotspotStyle._fontSize = pHighlightView->execute(SCI_STYLEGETSIZE, idStyle);

					int isBold = pHighlightView->execute(SCI_STYLEGETBOLD, idStyle);
					int isItalic = pHighlightView->execute(SCI_STYLEGETITALIC, idStyle);
					int isUnderline = pHighlightView->execute(SCI_STYLEGETUNDERLINE, idStyle);
					hotspotStyle._fontStyle = (isBold?FONTSTYLE_BOLD:0) | (isItalic?FONTSTYLE_ITALIC:0) | (isUnderline?FONTSTYLE_UNDERLINE:0);

					if (urlAction == 2)
						hotspotStyle._fontStyle |= FONTSTYLE_UNDERLINE;	//force underline

					//Apply style
					pHighlightView->execute(SCI_STYLESETFORE,		nextHotspotStyle, hotspotStyle._fgColor);
					pHighlightView->execute(SCI_STYLESETBACK,		nextHotspotStyle, hotspotStyle._bgColor);
					pHighlightView->execute(SCI_STYLESETFONT,		nextHotspotStyle, (LPARAM)hotspotStyle._fontName);
					pHighlightView->execute(SCI_STYLESETBOLD,		nextHotspotStyle, hotspotStyle._fontStyle & FONTSTYLE_BOLD);
					pHighlightView->execute(SCI_STYLESETITALIC,		nextHotspotStyle, hotspotStyle._fontStyle & FONTSTYLE_ITALIC);
					pHighlightView->execute(SCI_STYLESETUNDERLINE,	nextHotspotStyle, hotspotStyle._fontStyle & FONTSTYLE_UNDERLINE);
					pHighlightView->execute(SCI_STYLESETSIZE,		nextHotspotStyle, hotspotStyle._fontSize);
					pHighlightView->execute(SCI_STYLESETHOTSPOT,	nextHotspotStyle, TRUE);

					foundStyle = nextHotspotStyle;
					nextHotspotStyle--;	
				}
			}
			else	//if a style could be found, apply it
			{
				pHighlightView->execute(SCI_STARTSTYLING, targetStart, 0xFF);
				pHighlightView->execute(SCI_SETSTYLING, foundTextLen, foundStyle);
			}

 			int startMovingPos = targetStart + foundTextLen;

			if (startMovingPos >= endPos)
			{
				targetStart = -1;
			}
			else
			{
				pHighlightView->execute(SCI_SETTARGETSTART, startMovingPos);
				pHighlightView->execute(SCI_SETTARGETEND, endPos);
				targetStart = pHighlightView->execute(SCI_SEARCHINTARGET, strlen(urlRegExpressions[k]), (LPARAM)urlRegExpressions[k]);
			}
		}
	}
	pHighlightView->execute(SCI_STARTSTYLING, preEndStyled, 0xFF);
	
	// restore the original targets to avoid conflicts with the search/replace functions
	pHighlightView->execute(SCI_SETTARGETSTART, originalStartPos);
	pHighlightView->execute(SCI_SETTARGETEND, originalEndPos);
	_isDrawing = false;
}
*/
