// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "precompiledHeaders.h"

#include "UserDefineDialog.h"
#include "ScintillaEditView.h"
#include "Parameters.h"
#include "resource.h"
#include "Notepad_plus_msgs.h"
#include "FileDialog.h"

UserLangContainer * SharedParametersDialog::_pUserLang = NULL;
ScintillaEditView * SharedParametersDialog::_pScintilla = NULL;

void SharedParametersDialog::initControls()
{
	NppParameters *pNppParam = NppParameters::getInstance();
    for (int i = 0 ; i < _nbGroup ; i++)
    {
        HWND hFgColourStaticText = ::GetDlgItem(_hSelf, _fgStatic[i]);
        HWND hBgColourStaticText = ::GetDlgItem(_hSelf, _bgStatic[i]);
        
        _pFgColour[i] = new ColourPicker;
        _pFgColour[i]->init(_hInst, _hSelf);
        _pFgColour[i]->setColour(black);

        _pBgColour[i] = new ColourPicker;
        _pBgColour[i]->init(_hInst, _hSelf);
		_pBgColour[i]->setColour(white);

        POINT p1, p2;

        alignWith(hFgColourStaticText, _pFgColour[i]->getHSelf(), ALIGNPOS_RIGHT, p1);
        alignWith(hBgColourStaticText, _pBgColour[i]->getHSelf(), ALIGNPOS_RIGHT, p2);

        p1.x = p2.x = ((p1.x > p2.x)?p1.x:p2.x) + 10;
        p1.y -= 4; p2.y -= 4;

        ::MoveWindow(_pFgColour[i]->getHSelf(), p1.x, p1.y, 25, 25, TRUE);
        ::MoveWindow(_pBgColour[i]->getHSelf(), p2.x, p2.y, 25, 25, TRUE);
        _pFgColour[i]->display();
        _pBgColour[i]->display();
        
        //for the font size combos
        for(int j = 0 ; j < int(sizeof(fontSizeStrs))/(3*sizeof(TCHAR)) ; j++)
        {
			::SendDlgItemMessage(_hSelf, _fontSizeCombo[i], CB_ADDSTRING, 0, (LPARAM)fontSizeStrs[j]);
        }
        
        //for the font name combos
        HWND hFontNameCombo = ::GetDlgItem(_hSelf, _fontNameCombo[i]);
		
        const std::vector<generic_string> & fontlist = pNppParam->getFontList();
        for (int j = 0 ; j < int(fontlist.size()) ; j++)
        {
            int k = ::SendMessage(hFontNameCombo, CB_ADDSTRING, 0, (LPARAM)fontlist[j].c_str());
            ::SendMessage(hFontNameCombo, CB_SETITEMDATA, k, (LPARAM)fontlist[j].c_str());
        }
    }
}

bool SharedParametersDialog::setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set) 
{
	bool2set = (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, id), BM_GETCHECK, 0, 0));

	if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
		_pScintilla->styleChange();
	return TRUE;
}

void SharedParametersDialog::styleUpdate(const Style & style, ColourPicker *pFgColourPicker, ColourPicker *pBgColourPicker, 
										 int fontComboId, int fontSizeComboId, int boldCheckId, int italicCheckId, int underlineCheckId)
{
	pFgColourPicker->setColour((style._fgColor == COLORREF(-1))?black:style._fgColor);
	pFgColourPicker->setEnabled((style._colorStyle & COLORSTYLE_FOREGROUND) != 0);
	pFgColourPicker->redraw();
	pBgColourPicker->setColour((style._bgColor == COLORREF(-1))?white:style._bgColor);
	pBgColourPicker->setEnabled((style._colorStyle & COLORSTYLE_BACKGROUND) != 0);
	pBgColourPicker->redraw();

	HWND hFontCombo = ::GetDlgItem(_hSelf, fontComboId);
	int i = ::SendMessage(hFontCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)style._fontName);
	if (i == CB_ERR)
		i = 0;
	::SendMessage(hFontCombo, CB_SETCURSEL, i, 0);

	TCHAR size[10];
	if (style._fontSize == -1)
		size[0] = '\0';
	else
		wsprintf(size, TEXT("%d"), style._fontSize);

	hFontCombo = ::GetDlgItem(_hSelf, fontSizeComboId);
	i = ::SendMessage(hFontCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)size);
	if (i != CB_ERR)
		::SendMessage(hFontCombo, CB_SETCURSEL, i, 0);
	int isBold = 0;
	int isItalic = 0;
	int isUnderline = 0;
	if (style._fontStyle != -1)
	{
		isBold = (style._fontStyle & FONTSTYLE_BOLD)?BST_CHECKED:BST_UNCHECKED;
		isItalic = (style._fontStyle & FONTSTYLE_ITALIC)?BST_CHECKED:BST_UNCHECKED;
		isUnderline = (style._fontStyle & FONTSTYLE_UNDERLINE)?BST_CHECKED:BST_UNCHECKED;
	}
	::SendDlgItemMessage(_hSelf, boldCheckId, BM_SETCHECK, isBold, 0);
	::SendDlgItemMessage(_hSelf, italicCheckId, BM_SETCHECK, isItalic, 0);
	::SendDlgItemMessage(_hSelf, underlineCheckId, BM_SETCHECK, isUnderline, 0);
}

int fgStatic[] = {IDC_DEFAULT_FG_STATIC, IDC_FOLDEROPEN_FG_STATIC, IDC_FOLDERCLOSE_FG_STATIC};
int bgStatic[] = {IDC_DEFAULT_BG_STATIC, IDC_FOLDEROPEN_BG_STATIC, IDC_FOLDERCLOSE_BG_STATIC};
int fontSizeCombo[] = {IDC_DEFAULT_FONTSIZE_COMBO, IDC_FOLDEROPEN_FONTSIZE_COMBO, IDC_FOLDERCLOSE_FONTSIZE_COMBO};
int fontNameCombo[] = {IDC_DEFAULT_FONT_COMBO, IDC_FOLDEROPEN_FONT_COMBO, IDC_FOLDERCLOSE_FONT_COMBO};

FolderStyleDialog::FolderStyleDialog() : SharedParametersDialog(3) 
{
	memcpy(_fgStatic, fgStatic, sizeof(fgStatic));
	memcpy(_bgStatic, bgStatic, sizeof(bgStatic));
	memcpy(_fontSizeCombo, fontSizeCombo, sizeof(fontSizeCombo));
	memcpy(_fontNameCombo, fontNameCombo, sizeof(fontNameCombo));
}

void FolderStyleDialog::setKeywords2List(int ctrlID) 
{
    int index;
    if (ctrlID == IDC_FOLDEROPEN_EDIT)
        index = 1;
    else if (ctrlID == IDC_FOLDERCLOSE_EDIT)
        index = 2;
    else
        index = -1;
        
    if (index != -1)
		::GetDlgItemText(_hSelf, ctrlID, _pUserLang->_keywordLists[index], max_char);
}

int FolderStyleDialog::getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const
{
    switch (ctrlID)
    {
		case IDC_DEFAULT_FONT_COMBO :
            isFontSize = false;
            return STYLE_DEFAULT_INDEX;

        case IDC_DEFAULT_FONTSIZE_COMBO :
            isFontSize = true;
            return STYLE_DEFAULT_INDEX;

        case IDC_FOLDEROPEN_FONT_COMBO :
            isFontSize = false;
            return STYLE_BLOCK_OPEN_INDEX;

        case IDC_FOLDEROPEN_FONTSIZE_COMBO :
            isFontSize = true;
            return STYLE_BLOCK_OPEN_INDEX;

        case IDC_FOLDERCLOSE_FONT_COMBO :
            isFontSize = false;
            return STYLE_BLOCK_CLOSE_INDEX;

        case IDC_FOLDERCLOSE_FONTSIZE_COMBO :
            isFontSize = true;
            return STYLE_BLOCK_CLOSE_INDEX;

        default :
            return -1;
    }
}

BOOL CALLBACK SharedParametersDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			initControls();
            return TRUE;
		}

		case WM_COMMAND : 
		{
            if (HIWORD(wParam) == EN_CHANGE)
            {
                setKeywords2List(LOWORD(wParam));

                if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
                    _pScintilla->styleChange();

                return TRUE;
            }
			else if (HIWORD(wParam) == CBN_SELCHANGE)
            {
				bool isFontSize;
				int k = getGroupIndexFromCombo(LOWORD(wParam), isFontSize);

				if (k != -1)
				{
					int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
					Style & style = _pUserLang->_styleArray.getStyler(k);
					if (isFontSize)
					{
						TCHAR intStr[5];
						if (i != 0)
						{
							::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETLBTEXT, i, (LPARAM)intStr);
							if (!intStr[0])
								style._fontSize = -1;
							else
							{
								TCHAR *finStr;
								style._fontSize = generic_strtol(intStr, &finStr, 10);
								if (*finStr != '\0')
									style._fontSize = -1;
							}
						}
					}
					else
					{
						style._fontName = (TCHAR *)::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETITEMDATA, i, 0);
					}
					if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
						_pScintilla->styleChange();
					return TRUE;
				}
			}
			else if (HIWORD(wParam) == CPN_COLOURPICKED)
			{
                bool isFG;
                ColourPicker *pCP;
                int index = getStylerIndexFromCP((HWND)lParam, isFG, &pCP);
                if (index != -1)
                {
                    Style & style = _pUserLang->_styleArray.getStyler(index);
					if (isFG) {
						style._fgColor = pCP->getColour();
						if (pCP->isEnabled())
							style._colorStyle |= COLORSTYLE_FOREGROUND;
						else
							style._colorStyle &= ~COLORSTYLE_FOREGROUND;
					} else {
						style._bgColor = pCP->getColour();
						if (pCP->isEnabled())
							style._colorStyle |= COLORSTYLE_BACKGROUND;
						else
							style._colorStyle &= ~COLORSTYLE_BACKGROUND;
					}
				}
				// A cause de "#define CPN_COLOURPICKED (BN_CLICKED)"
				// Nous sommes obligés de mettre ce bloc ici !!!
				// A modifier !!!
				else
				{
					int fontStyleMask;
					int k = getGroupeIndexFromCheck(wParam, fontStyleMask);

					if (k != -1)
					{
						Style & style = _pUserLang->_styleArray.getStyler(k);
						if (style._fontStyle == -1)
								style._fontStyle = 0;
						style._fontStyle ^= fontStyleMask;
						//::MessageBox(NULL, TEXT("Bingo!!!"), TEXT(""), MB_OK);
					}
				}
				if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
					_pScintilla->styleChange();
                return TRUE;
			}
			return FALSE;
		}
		/*
		case WM_SIZE :
		{
			redraw();
			return TRUE;
		}
		*/
		case WM_DESTROY:
		{
			for (int i = 0 ; i < _nbGroup ; i++)
			{
				_pFgColour[i]->destroy();
				_pBgColour[i]->destroy();
				
				delete _pFgColour[i];
				delete _pBgColour[i];
			}
			return TRUE;
		}
	}
	return FALSE;
}

void FolderStyleDialog::updateDlg() 
{
	::SendDlgItemMessage(_hSelf, IDC_FOLDEROPEN_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[KWL_FOLDER_OPEN_INDEX]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDERCLOSE_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[KWL_FOLDER_CLOSE_INDEX]));

	Style & defaultStyle = _pUserLang->_styleArray.getStyler(STYLE_DEFAULT_INDEX);
	styleUpdate(defaultStyle, _pFgColour[0], _pBgColour[0], IDC_DEFAULT_FONT_COMBO, IDC_DEFAULT_FONTSIZE_COMBO,
		 IDC_DEFAULT_BOLD_CHECK, IDC_DEFAULT_ITALIC_CHECK, IDC_DEFAULT_UNDERLINE_CHECK);

	Style & foStyle = _pUserLang->_styleArray.getStyler(STYLE_BLOCK_OPEN_INDEX);
	styleUpdate(foStyle, _pFgColour[1], _pBgColour[1], IDC_FOLDEROPEN_FONT_COMBO, IDC_FOLDEROPEN_FONTSIZE_COMBO,
		 IDC_FOLDEROPEN_BOLD_CHECK, IDC_FOLDEROPEN_ITALIC_CHECK, IDC_FOLDEROPEN_UNDERLINE_CHECK);

	Style & fcStyle = _pUserLang->_styleArray.getStyler(STYLE_BLOCK_CLOSE_INDEX);
	styleUpdate(fcStyle, _pFgColour[2], _pBgColour[2], IDC_FOLDERCLOSE_FONT_COMBO, IDC_FOLDERCLOSE_FONTSIZE_COMBO, 
		 IDC_FOLDERCLOSE_BOLD_CHECK, IDC_FOLDERCLOSE_ITALIC_CHECK, IDC_FOLDERCLOSE_UNDERLINE_CHECK);
}

int FolderStyleDialog::getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const
{
    for (int i = 0 ; i < _nbGroup ; i++)
    {
        if (hWnd == _pFgColour[i]->getHSelf())
        {
            *ppCP = _pFgColour[i];
            isFG = true;
            return i;
        }
        if (hWnd == _pBgColour[i]->getHSelf())
        {
            *ppCP = _pBgColour[i];
            isFG = false;
            return i;
        }
    }
    return -1;
}

int FolderStyleDialog::getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const 
{
    switch (ctrlID)
    {
        case IDC_DEFAULT_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_DEFAULT_INDEX;

        case IDC_DEFAULT_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_DEFAULT_INDEX;

		case IDC_DEFAULT_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_DEFAULT_INDEX;

        case IDC_FOLDEROPEN_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_BLOCK_OPEN_INDEX;

        case IDC_FOLDEROPEN_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_BLOCK_OPEN_INDEX;

		case IDC_FOLDEROPEN_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_BLOCK_OPEN_INDEX;

        case IDC_FOLDERCLOSE_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_BLOCK_CLOSE_INDEX;

        case IDC_FOLDERCLOSE_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_BLOCK_CLOSE_INDEX;

		case IDC_FOLDERCLOSE_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_BLOCK_CLOSE_INDEX;

        default :
            return -1;
    }
}

int fgStatic2[] = {IDC_KEYWORD1_FG_STATIC, IDC_KEYWORD2_FG_STATIC, IDC_KEYWORD3_FG_STATIC, IDC_KEYWORD4_FG_STATIC};
int bgStatic2[] = {IDC_KEYWORD1_BG_STATIC, IDC_KEYWORD2_BG_STATIC, IDC_KEYWORD3_BG_STATIC, IDC_KEYWORD4_BG_STATIC};
int fontSizeCombo2[] = {IDC_KEYWORD1_FONTSIZE_COMBO, IDC_KEYWORD2_FONTSIZE_COMBO, IDC_KEYWORD3_FONTSIZE_COMBO, IDC_KEYWORD4_FONTSIZE_COMBO};
int fontNameCombo2[] = {IDC_KEYWORD1_FONT_COMBO, IDC_KEYWORD2_FONT_COMBO, IDC_KEYWORD3_FONT_COMBO, IDC_KEYWORD4_FONT_COMBO};

KeyWordsStyleDialog::KeyWordsStyleDialog() : SharedParametersDialog(4) 
{
	memcpy(_fgStatic, fgStatic2, sizeof(fgStatic2));
	memcpy(_bgStatic, bgStatic2, sizeof(bgStatic2));
	memcpy(_fontSizeCombo, fontSizeCombo2, sizeof(fontSizeCombo2));
	memcpy(_fontNameCombo, fontNameCombo2, sizeof(fontNameCombo2));
}


BOOL CALLBACK KeyWordsStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message) 
	{

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_KEYWORD1_PREFIX_CHECK :
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[0]);

				case IDC_KEYWORD2_PREFIX_CHECK :
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[1]);

				case IDC_KEYWORD3_PREFIX_CHECK :
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[2]);

				case IDC_KEYWORD4_PREFIX_CHECK :
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[3]);
			}
		}
		default :
			return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}

void KeyWordsStyleDialog::setKeywords2List(int id) 
{
	int index;
	switch (id)
	{
		case IDC_KEYWORD1_EDIT : index = 5; break;
		case IDC_KEYWORD2_EDIT : index = 6; break;
		case IDC_KEYWORD3_EDIT : index = 7; break;
		case IDC_KEYWORD4_EDIT : index = 8; break;
		default : index = -1;
	}
    if (index != -1)
		::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[index], max_char);
}

int KeyWordsStyleDialog::getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const
{
    for (int i = 0 ; i < _nbGroup ; i++)
    {
        if (hWnd == _pFgColour[i]->getHSelf())
        {
            *ppCP = _pFgColour[i];
            isFG = true;
            return i+3;
        }
        if (hWnd == _pBgColour[i]->getHSelf())
        {
            *ppCP = _pBgColour[i];
            isFG = false;
            return i+3;
        }
    }
    return -1;
}

 int KeyWordsStyleDialog::getGroupIndexFromCombo(int ctrlID, bool & isFontSize)  const 
 {
    switch (ctrlID)
	{
		case IDC_KEYWORD1_FONT_COMBO :
            isFontSize = false;
            return STYLE_WORD1_INDEX;

		case IDC_KEYWORD1_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_WORD1_INDEX;

		case IDC_KEYWORD2_FONT_COMBO : 
            isFontSize = false;
            return STYLE_WORD2_INDEX;

		case IDC_KEYWORD2_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_WORD2_INDEX;

        case IDC_KEYWORD3_FONT_COMBO : 
            isFontSize = false;
            return STYLE_WORD3_INDEX;

		case IDC_KEYWORD3_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_WORD3_INDEX;

		case IDC_KEYWORD4_FONT_COMBO : 
            isFontSize = false;
            return STYLE_WORD4_INDEX;

		case IDC_KEYWORD4_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_WORD4_INDEX;

		default : 
            return -1;
	}
}

int KeyWordsStyleDialog::getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const 
{
	switch (ctrlID)
    {
        case IDC_KEYWORD1_BOLD_CHECK :
			fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_WORD1_INDEX;

        case IDC_KEYWORD1_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_WORD1_INDEX;

		case IDC_KEYWORD1_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_WORD1_INDEX;

        case IDC_KEYWORD2_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_WORD2_INDEX;

        case IDC_KEYWORD2_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_WORD2_INDEX;

		case IDC_KEYWORD2_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_WORD2_INDEX;

        case IDC_KEYWORD3_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_WORD3_INDEX;

        case IDC_KEYWORD3_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_WORD3_INDEX;

		case IDC_KEYWORD3_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_WORD3_INDEX;

        case IDC_KEYWORD4_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_WORD4_INDEX;

        case IDC_KEYWORD4_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_WORD4_INDEX;

		case IDC_KEYWORD4_UNDERLINE_CHECK :
			fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_WORD4_INDEX;

        default :
            return -1;
    }
}

void KeyWordsStyleDialog::updateDlg() 
{
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD1_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[KWL_KW1_INDEX]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD2_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[KWL_KW2_INDEX]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD3_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[KWL_KW3_INDEX]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD4_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[KWL_KW4_INDEX]));

	Style & w1Style = _pUserLang->_styleArray.getStyler(STYLE_WORD1_INDEX);
	styleUpdate(w1Style, _pFgColour[0], _pBgColour[0], IDC_KEYWORD1_FONT_COMBO, IDC_KEYWORD1_FONTSIZE_COMBO,
		 IDC_KEYWORD1_BOLD_CHECK, IDC_KEYWORD1_ITALIC_CHECK, IDC_KEYWORD1_UNDERLINE_CHECK);

	Style & w2Style = _pUserLang->_styleArray.getStyler(STYLE_WORD2_INDEX);
	styleUpdate(w2Style, _pFgColour[1], _pBgColour[1], IDC_KEYWORD2_FONT_COMBO, IDC_KEYWORD2_FONTSIZE_COMBO,
		 IDC_KEYWORD2_BOLD_CHECK, IDC_KEYWORD2_ITALIC_CHECK, IDC_KEYWORD2_UNDERLINE_CHECK);

	Style & w3Style = _pUserLang->_styleArray.getStyler(STYLE_WORD3_INDEX);
	styleUpdate(w3Style, _pFgColour[2], _pBgColour[2], IDC_KEYWORD3_FONT_COMBO, IDC_KEYWORD3_FONTSIZE_COMBO,
		 IDC_KEYWORD3_BOLD_CHECK, IDC_KEYWORD3_BOLD_CHECK, IDC_KEYWORD3_UNDERLINE_CHECK);

	Style & w4Style = _pUserLang->_styleArray.getStyler(STYLE_WORD4_INDEX);
	styleUpdate(w4Style, _pFgColour[3], _pBgColour[3], IDC_KEYWORD4_FONT_COMBO, IDC_KEYWORD4_FONTSIZE_COMBO,
		 IDC_KEYWORD4_BOLD_CHECK, IDC_KEYWORD4_ITALIC_CHECK, IDC_KEYWORD4_UNDERLINE_CHECK);

	::SendDlgItemMessage(_hSelf, IDC_KEYWORD1_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[0], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD2_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[1], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD3_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[2], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD4_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[3], 0);
}

int fgStatic3[] = {IDC_COMMENT_FG_STATIC, IDC_COMMENTLINE_FG_STATIC, IDC_NUMBER_FG_STATIC};
int bgStatic3[] = {IDC_COMMENT_BG_STATIC, IDC_COMMENTLINE_BG_STATIC, IDC_NUMBER_BG_STATIC};
int fontSizeCombo3[] = {IDC_COMMENT_FONTSIZE_COMBO, IDC_COMMENTLINE_FONTSIZE_COMBO, IDC_NUMBER_FONTSIZE_COMBO};
int fontNameCombo3[] = {IDC_COMMENT_FONT_COMBO, IDC_COMMENTLINE_FONT_COMBO, IDC_NUMBER_FONT_COMBO};

CommentStyleDialog::CommentStyleDialog() : SharedParametersDialog(3)
{
    memcpy(_fgStatic, fgStatic3, sizeof(fgStatic3));
	memcpy(_bgStatic, bgStatic3, sizeof(bgStatic3));
	memcpy(_fontSizeCombo, fontSizeCombo3, sizeof(fontSizeCombo3));
	memcpy(_fontNameCombo, fontNameCombo3, sizeof(fontNameCombo3));
}

BOOL CALLBACK CommentStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message) 
	{
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDC_COMMENTLINESYMBOL_CHECK :
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isCommentLineSymbol);;

				case IDC_COMMENTSYMBOL_CHECK :
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isCommentSymbol);;

			}
		}
		default :
			return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}


void CommentStyleDialog::setKeywords2List(int id) 
{
    int i;
    switch (id)
    {
        case IDC_COMMENTOPEN_EDIT : 
        case IDC_COMMENTCLOSE_EDIT : 
        case IDC_COMMENTLINE_EDIT : 
            i = 4;
            break;
        default : i = -1;
    }
    if (i != -1)
    {
        TCHAR commentOpen[max_char];
        TCHAR commentClose[max_char];
        TCHAR commentLine[max_char];
        TCHAR newList[max_char] = TEXT("");
        ::GetDlgItemText(_hSelf, IDC_COMMENTOPEN_EDIT, commentOpen, max_char);
        ::GetDlgItemText(_hSelf, IDC_COMMENTCLOSE_EDIT, commentClose, max_char);
        ::GetDlgItemText(_hSelf, IDC_COMMENTLINE_EDIT, commentLine, max_char);
        convertTo(newList, commentOpen, '1');
        convertTo(newList, commentClose, '2');
        convertTo(newList, commentLine, '0');
        lstrcpy(_pUserLang->_keywordLists[i], newList);
    }
}

int CommentStyleDialog::getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const
{
    for (int i = 0 ; i < _nbGroup ; i++)
    {
        if (hWnd == _pFgColour[i]->getHSelf())
        {
            *ppCP = _pFgColour[i];
            isFG = true;
            return i+7;
        }
        if (hWnd == _pBgColour[i]->getHSelf())
        {
            *ppCP = _pBgColour[i];
            isFG = false;
            return i+7;
        }
    }
    return -1;
}

int CommentStyleDialog::getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const 
{
    switch (ctrlID)
	{
		case IDC_COMMENT_FONT_COMBO :
            isFontSize = false;
            return STYLE_COMMENT_INDEX;
		
		case IDC_COMMENT_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_COMMENT_INDEX;            
		
		case IDC_COMMENTLINE_FONT_COMBO : 
            isFontSize = false;
            return STYLE_COMMENTLINE_INDEX;
		
		case IDC_COMMENTLINE_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_COMMENTLINE_INDEX;
		
		case IDC_NUMBER_FONT_COMBO : 
            isFontSize = false;
            return STYLE_NUMBER_INDEX;
		
		case IDC_NUMBER_FONTSIZE_COMBO : 
            isFontSize = true;
            return STYLE_NUMBER_INDEX;

		default : 
            return -1;
	}
}

int CommentStyleDialog::getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const 
{
	switch (ctrlID)
    {
        case IDC_COMMENT_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_COMMENT_INDEX;
        
		case IDC_COMMENT_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_COMMENT_INDEX;

        case IDC_COMMENT_UNDERLINE_CHECK :
            fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_COMMENT_INDEX;

		case IDC_COMMENTLINE_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_COMMENTLINE_INDEX;
        
		case IDC_COMMENTLINE_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_COMMENTLINE_INDEX;
        
        case IDC_COMMENTLINE_UNDERLINE_CHECK :
            fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_COMMENTLINE_INDEX;

		case IDC_NUMBER_BOLD_CHECK :
            fontStyleMask = FONTSTYLE_BOLD;
            return STYLE_NUMBER_INDEX;
        
		case IDC_NUMBER_ITALIC_CHECK :
            fontStyleMask = FONTSTYLE_ITALIC;
            return STYLE_NUMBER_INDEX;

        case IDC_NUMBER_UNDERLINE_CHECK :
            fontStyleMask = FONTSTYLE_UNDERLINE;
            return STYLE_NUMBER_INDEX;

        default :
            return -1;
    }
}

void CommentStyleDialog::convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR prefix) const 
{
    int index = lstrlen(dest);
    dest[index++] = ' ';
    dest[index++] = prefix;
    for (int i = 0 ; i < int(lstrlen(toConvert)) ; i++)
    {
        if (toConvert[i] == ' ')
        {
            if (toConvert[i+1] != ' ')
            {
                dest[index++] = ' ';
                dest[index++] = prefix;
            }
        }
        else
        {
            dest[index++] = toConvert[i];
        }
    }
    dest[index] = '\0'; 
}

void CommentStyleDialog::retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR prefix) const 
{
	int j = 0;
	bool begin2Copy = false;

	for (int i = 0 ; i < int(lstrlen(toRetrieve)) ; i++)
	{
		if (((i == 0) || toRetrieve[i-1] == ' ') && (toRetrieve[i] == prefix))
		{
			begin2Copy = true;
			continue;
		}
		else if (((toRetrieve[i] == ' ') && begin2Copy == true))
		{
			dest[j++] = toRetrieve[i];
			begin2Copy = false;
		}
		if (begin2Copy)
			dest[j++] = toRetrieve[i];
	}
	dest[j++] = '\0';
}

void CommentStyleDialog::updateDlg()
{
	TCHAR commentOpen[256] = TEXT("");
	TCHAR commentClose[256] = TEXT("");
	TCHAR commentLine[256] = TEXT("");

	retrieve(commentOpen, _pUserLang->_keywordLists[KWL_COMMENT_INDEX], '1');
	retrieve(commentClose, _pUserLang->_keywordLists[KWL_COMMENT_INDEX], '2');
	retrieve(commentLine, _pUserLang->_keywordLists[KWL_COMMENT_INDEX], '0');

	::SendDlgItemMessage(_hSelf, IDC_COMMENTOPEN_EDIT, WM_SETTEXT, 0, (LPARAM)commentOpen);
	::SendDlgItemMessage(_hSelf, IDC_COMMENTCLOSE_EDIT, WM_SETTEXT, 0, (LPARAM)commentClose);
	::SendDlgItemMessage(_hSelf, IDC_COMMENTLINE_EDIT, WM_SETTEXT, 0, (LPARAM)commentLine);

	Style & commentStyle = _pUserLang->_styleArray.getStyler(STYLE_COMMENT_INDEX);
	styleUpdate(commentStyle, _pFgColour[0], _pBgColour[0], IDC_COMMENT_FONT_COMBO, IDC_COMMENT_FONTSIZE_COMBO,
		 IDC_COMMENT_BOLD_CHECK, IDC_COMMENT_ITALIC_CHECK, IDC_COMMENT_UNDERLINE_CHECK);

	Style & commentLineStyle = _pUserLang->_styleArray.getStyler(STYLE_COMMENTLINE_INDEX);
	styleUpdate(commentLineStyle, _pFgColour[1], _pBgColour[1], IDC_COMMENTLINE_FONT_COMBO, IDC_COMMENTLINE_FONTSIZE_COMBO,
		 IDC_COMMENTLINE_BOLD_CHECK, IDC_COMMENTLINE_ITALIC_CHECK, IDC_COMMENTLINE_UNDERLINE_CHECK);

	Style & numberStyle = _pUserLang->_styleArray.getStyler(STYLE_NUMBER_INDEX);
	styleUpdate(numberStyle, _pFgColour[2], _pBgColour[2], IDC_NUMBER_FONT_COMBO, IDC_NUMBER_FONTSIZE_COMBO,
		 IDC_NUMBER_BOLD_CHECK, IDC_NUMBER_ITALIC_CHECK, IDC_NUMBER_UNDERLINE_CHECK);

	::SendDlgItemMessage(_hSelf, IDC_COMMENTLINESYMBOL_CHECK, BM_SETCHECK, _pUserLang->_isCommentLineSymbol, 0);
	::SendDlgItemMessage(_hSelf, IDC_COMMENTSYMBOL_CHECK, BM_SETCHECK, _pUserLang->_isCommentSymbol, 0);
}

TCHAR symbolesArray[] = TEXT("+-*/.?!:;,%^$&\"'(_)=}]@\\`|[{#~<>");
const bool SymbolsStyleDialog::ADD = true;
const bool SymbolsStyleDialog::REMOVE = false;

int fgStatic4[] = {IDC_SYMBOL_FG_STATIC, IDC_SYMBOL_FG2_STATIC, IDC_SYMBOL_FG3_STATIC};
int bgStatic4[] = {IDC_SYMBOL_BG_STATIC, IDC_SYMBOL_BG2_STATIC, IDC_SYMBOL_BG3_STATIC};
int fontSizeCombo4[] = {IDC_SYMBOL_FONTSIZE_COMBO, IDC_SYMBOL_FONTSIZE2_COMBO, IDC_SYMBOL_FONTSIZE3_COMBO};
int fontNameCombo4[] = {IDC_SYMBOL_FONT_COMBO, IDC_SYMBOL_FONT2_COMBO, IDC_SYMBOL_FONT3_COMBO};

// 2 static const TCHAR * to have the compatibility with the old xml
const TCHAR *SymbolsStyleDialog::_delimTag1 = TEXT("DELIMINER1");
const TCHAR *SymbolsStyleDialog::_delimTag2 = TEXT("DELIMINER2");

SymbolsStyleDialog::SymbolsStyleDialog() : SharedParametersDialog(3)
{
    memcpy(_fgStatic, fgStatic4, sizeof(fgStatic4));
	memcpy(_bgStatic, bgStatic4, sizeof(bgStatic4));
	memcpy(_fontSizeCombo, fontSizeCombo4, sizeof(fontSizeCombo4));
	memcpy(_fontNameCombo, fontNameCombo4, sizeof(fontNameCombo4));
}

int SymbolsStyleDialog::getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const
{
    for (int i = 0 ; i < _nbGroup ; i++)
    {
		if (hWnd == _pFgColour[i]->getHSelf())
		{
			*ppCP = _pFgColour[i];
			isFG = true;
			return i+10;
		}
		if (hWnd == _pBgColour[i]->getHSelf())
		{
			*ppCP = _pBgColour[i];
			isFG = false;
			return i+10;
		}
	}
    return -1;
}

int SymbolsStyleDialog::getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const 
{
	switch (ctrlID)
	{
		case IDC_SYMBOL_FONT_COMBO :
			isFontSize = false;
			return STYLE_OPERATOR_INDEX;

		case IDC_SYMBOL_FONTSIZE_COMBO :
			isFontSize = true;
			return STYLE_OPERATOR_INDEX;

		case IDC_SYMBOL_FONT2_COMBO :
			isFontSize = false;
			return STYLE_DELIM2_INDEX;

		case IDC_SYMBOL_FONTSIZE2_COMBO :
			isFontSize = true;
			return STYLE_DELIM2_INDEX;

		case IDC_SYMBOL_FONT3_COMBO :
			isFontSize = false;
			return STYLE_DELIM3_INDEX;

		case IDC_SYMBOL_FONTSIZE3_COMBO :
			isFontSize = true;
			return STYLE_DELIM3_INDEX;

		default : 
            return -1;
	}
}

void SymbolsStyleDialog::symbolAction(bool action)
{
	int id2Add, id2Remove;
	int idButton2Disable, idButton2Enable;
	if (action == ADD)
	{
		id2Add = IDC_ACTIVATED_SYMBOL_LIST;
		id2Remove = IDC_AVAILABLE_SYMBOLS_LIST;
		idButton2Enable = IDC_REMOVE_BUTTON;
		idButton2Disable = IDC_ADD_BUTTON;
		
	}
	else
	{
		id2Add = IDC_AVAILABLE_SYMBOLS_LIST;
		id2Remove = IDC_ACTIVATED_SYMBOL_LIST;
		idButton2Enable = IDC_ADD_BUTTON;
		idButton2Disable = IDC_REMOVE_BUTTON;
	}
	int i = ::SendDlgItemMessage(_hSelf, id2Remove, LB_GETCURSEL, 0, 0);
	TCHAR s[2];
	::SendDlgItemMessage(_hSelf, id2Remove, LB_GETTEXT, i, (LPARAM)s);

	::SendDlgItemMessage(_hSelf, id2Add, LB_ADDSTRING, 0, (LPARAM)s);
	::SendDlgItemMessage(_hSelf, id2Remove, LB_DELETESTRING, i, 0);
	int count = ::SendDlgItemMessage(_hSelf, id2Remove, LB_GETCOUNT, 0, 0);
	if (i == count)
		i -= 1;
		
	::SendDlgItemMessage(_hSelf, id2Remove, LB_SETCURSEL, i, 0);
	count = ::SendDlgItemMessage(_hSelf, id2Remove, LB_GETCOUNT, 0, 0);

	// If there's no symbol, we activate another side
	if (!count)
	{
		::SendDlgItemMessage(_hSelf, id2Add, LB_SETCURSEL, 0, 0);
		::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
		::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);
	}

	// Get the operators list
	count = ::SendDlgItemMessage(_hSelf, IDC_ACTIVATED_SYMBOL_LIST, LB_GETCOUNT, 0, 0);

	int j = 0;
	for (int i = 0 ; i < count ; i++)
	{
		::SendDlgItemMessage(_hSelf, IDC_ACTIVATED_SYMBOL_LIST, LB_GETTEXT, i, (LPARAM)s);
		_pUserLang->_keywordLists[3][j++] = s[0];
		_pUserLang->_keywordLists[3][j++] = ' ';
	}
	_pUserLang->_keywordLists[3][--j] = '\0';
	
	if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
		_pScintilla->styleChange();
}

void SymbolsStyleDialog::listboxsRemoveAll()
{
	int count = ::SendDlgItemMessage(_hSelf, IDC_AVAILABLE_SYMBOLS_LIST, LB_GETCOUNT, 0, 0);
	for (int i = count-1 ; i >= 0 ; i--)
	{
		::SendDlgItemMessage(_hSelf, IDC_AVAILABLE_SYMBOLS_LIST, LB_DELETESTRING, i, 0);
	}
	count = ::SendDlgItemMessage(_hSelf, IDC_ACTIVATED_SYMBOL_LIST, LB_GETCOUNT, 0, 0);
	for (int i = count-1 ; i >= 0 ; i--)
	{
		::SendDlgItemMessage(_hSelf, IDC_ACTIVATED_SYMBOL_LIST, LB_DELETESTRING, i, 0);
	}
}
void SymbolsStyleDialog::updateDlg() 
{
	listboxsReInit();

	const TCHAR *symbols = _pUserLang->_keywordLists[KWL_OPERATOR_INDEX];

	for (int i = 0 ; i < int(lstrlen(symbols)) ; i++)
	{
		if (symbols[i] != ' ')
		{
			TCHAR s[2];
			s[0] = symbols[i];
			s[1] = '\0';
			int index = ::SendDlgItemMessage(_hSelf, IDC_AVAILABLE_SYMBOLS_LIST, LB_FINDSTRING, (WPARAM)-1, (LPARAM)s);
			if (index == LB_ERR)
				continue;

			int id2Add = IDC_ACTIVATED_SYMBOL_LIST;
			int id2Remove = IDC_AVAILABLE_SYMBOLS_LIST;
			int idButton2Enable = IDC_REMOVE_BUTTON;
			int idButton2Disable = IDC_ADD_BUTTON;

			::SendDlgItemMessage(_hSelf, id2Add, LB_ADDSTRING, 0, (LPARAM)s);
			::SendDlgItemMessage(_hSelf, id2Remove, LB_DELETESTRING, index, 0);
			int count = ::SendDlgItemMessage(_hSelf, id2Remove, LB_GETCOUNT, 0, 0);
			if (index == count)
				index -= 1;

			::SendDlgItemMessage(_hSelf, id2Remove, LB_SETCURSEL, index, 0);
			count = ::SendDlgItemMessage(_hSelf, id2Remove, LB_GETCOUNT, 0, 0);

			// If there's no symbol, we activate another side
			if (!count)
			{
				::SendDlgItemMessage(_hSelf, id2Add, LB_SETCURSEL, 0, 0);
				::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
				::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);
			}
		}
	}
	bool hasEscape = (_pUserLang->_escapeChar[0] != 0);
	::SendDlgItemMessage(_hSelf, IDC_HAS_ESCAPE, BM_SETCHECK, (hasEscape) ? BST_CHECKED : BST_UNCHECKED,0);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_ESCAPE_CHAR), (hasEscape) ? TRUE : FALSE);
	::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(&_pUserLang->_escapeChar));
	const TCHAR *delims = _pUserLang->_keywordLists[KWL_DELIM_INDEX];
	// ICI LE TRAITEMENT POUR REMPLIR LES 4 COMBO BOX
	TCHAR dOpen1[2], dClose1[2], dOpen2[2], dClose2[2], dOpen3[2], dClose3[2];
	dOpen1[0] = dClose1[0] = dOpen2[0] = dClose2[0] = dOpen3[0] = dClose3[0] = '\0';
	dOpen1[1] = dClose1[1] = dOpen2[1] = dClose2[1] = dOpen3[1] = dClose3[1] = '\0';
	if (lstrlen(delims) >= 6)
	{
		if (delims[0] != '0')
			dOpen1[0] = delims[0];

		int i = ::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO2_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)dOpen1);
		if (i == CB_ERR)
			i = 0;
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO2_COMBO,CB_SETCURSEL, i, 0);

		if (delims[1] != '0')
			dOpen2[0] = delims[1];

		i = ::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO3_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)dOpen2);
		if (i == CB_ERR)
			i = 0;
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO3_COMBO,CB_SETCURSEL, i, 0);

		//if (delims[2] != '0')
			//dOpen3 = delims[2];
		if (delims[3] != '0')
			dClose1[0] = delims[3];

		i = ::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC2_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)dClose1);
		if (i == CB_ERR)
			i = 0;
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC2_COMBO,CB_SETCURSEL, i, 0);
		
		if (delims[4] != '0')
			dClose2[0] = delims[4];

		i = ::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC3_COMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)dClose2);
		if (i == CB_ERR)
			i = 0;
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC3_COMBO,CB_SETCURSEL, i, 0);
		//if (delims[5] != '0')
			//dClose3 = delims[5];
	}


	Style & opStyle = _pUserLang->_styleArray.getStyler(STYLE_OPERATOR_INDEX);
	
	styleUpdate(opStyle, _pFgColour[0], _pBgColour[0], IDC_SYMBOL_FONT_COMBO, IDC_SYMBOL_FONTSIZE_COMBO,
		 IDC_SYMBOL_BOLD_CHECK, IDC_SYMBOL_ITALIC_CHECK, IDC_SYMBOL_UNDERLINE_CHECK);
		 
	Style & delim2Style = _pUserLang->_styleArray.getStyler(STYLE_DELIM2_INDEX);
	
	// the compatibility with the old xml
	if (delim2Style._styleID == -1)
	{
		delim2Style._styleID = SCE_USER_DELIMITER1;
		delim2Style._styleDesc = SymbolsStyleDialog::_delimTag1;
	}
	styleUpdate(delim2Style, _pFgColour[1], _pBgColour[1], IDC_SYMBOL_FONT2_COMBO, IDC_SYMBOL_FONTSIZE2_COMBO,
		 IDC_SYMBOL_BOLD2_CHECK, IDC_SYMBOL_ITALIC2_CHECK, IDC_SYMBOL_UNDERLINE2_CHECK);
		 
	Style & delim3Style = _pUserLang->_styleArray.getStyler(STYLE_DELIM3_INDEX);

	// the compatibility with the old xml
	if (delim3Style._styleID == -1)
	{
		delim3Style._styleID = SCE_USER_DELIMITER2;
		delim3Style._styleDesc = SymbolsStyleDialog::_delimTag2;
	}
	styleUpdate(delim3Style, _pFgColour[2], _pBgColour[2], IDC_SYMBOL_FONT3_COMBO, IDC_SYMBOL_FONTSIZE3_COMBO,
		 IDC_SYMBOL_BOLD3_CHECK, IDC_SYMBOL_ITALIC3_CHECK, IDC_SYMBOL_UNDERLINE3_CHECK);

	// the compatibility with the old xml
	if (_pUserLang->_styleArray.getNbStyler() < 13)
		_pUserLang->_styleArray.setNbStyler(13);
}

void SymbolsStyleDialog::listboxsInit() 
{
	::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO2_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT(""));
	::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC2_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT(""));
	::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO3_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT(""));
	::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC3_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT(""));

	for (int i = 0 ; i < int((sizeof(symbolesArray)/sizeof(TCHAR))-1) ; i++)
	{
		TCHAR s[2];
		s[0] = symbolesArray[i];
		s[1] = '\0';
		::SendDlgItemMessage(_hSelf, IDC_AVAILABLE_SYMBOLS_LIST, LB_ADDSTRING, 0, (LPARAM)s);
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO2_COMBO, CB_ADDSTRING, 0, (LPARAM)s);
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC2_COMBO, CB_ADDSTRING, 0, (LPARAM)s);
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BO3_COMBO, CB_ADDSTRING, 0, (LPARAM)s);
		::SendDlgItemMessage(_hSelf, IDC_SYMBOL_BC3_COMBO, CB_ADDSTRING, 0, (LPARAM)s);
	}
}

BOOL CALLBACK SymbolsStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			// 2 listBoxes et 4 combobox
			listboxsInit();

			::SendDlgItemMessage(_hSelf, IDC_AVAILABLE_SYMBOLS_LIST, LB_SETCURSEL, 0, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_REMOVE_BUTTON), FALSE);
			::SendDlgItemMessage(_hSelf,IDC_ESCAPE_CHAR, EM_LIMITTEXT,1,0);

			return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
		}

		case WM_COMMAND : 
		{
			//int toto = HIWORD(wParam);
			if ((wParam == IDC_ADD_BUTTON) || (wParam == IDC_REMOVE_BUTTON))
			{
				symbolAction((wParam == IDC_ADD_BUTTON)?ADD:REMOVE);
				if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
					_pScintilla->styleChange();
				return TRUE;
			}
			else if (wParam == IDC_HAS_ESCAPE)
			{
				int newState = ::SendDlgItemMessage(_hSelf,IDC_HAS_ESCAPE, BM_GETCHECK, 0, 0);
				::EnableWindow(::GetDlgItem(_hSelf, IDC_ESCAPE_CHAR), (newState == BST_CHECKED) ? TRUE : FALSE);
				if ((newState == BST_CHECKED) && !::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_GETTEXTLENGTH, 0, 0) && (_lastEscapeChar != 0))
				//restore previous char
				{
					_pUserLang->_escapeChar[0] = _lastEscapeChar;
					::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(&_pUserLang->_escapeChar[0]));
				}
				else
				{
					_lastEscapeChar = _pUserLang->_escapeChar[0];
					::SendDlgItemMessage(_hSelf,IDC_ESCAPE_CHAR,WM_SETTEXT,0,reinterpret_cast<LPARAM>(""));
					_pUserLang->_escapeChar[0]='\0';
				}
				if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
				{
					_pScintilla->execute(SCI_SETPROPERTY, (WPARAM)"userDefine.escapeChar", reinterpret_cast<LPARAM>(_pUserLang->_escapeChar)); 
					_pScintilla->styleChange();
				}
			}
				
			else if (LOWORD(wParam) == IDC_ESCAPE_CHAR)
			{
				if (HIWORD(wParam) == EN_CHANGE)
				{
					if (::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_GETTEXTLENGTH, 0, 0))
					{
						::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_GETTEXT, sizeof(_pUserLang->_escapeChar), reinterpret_cast<LPARAM>(&_pUserLang->_escapeChar));
						_lastEscapeChar = _pUserLang->_escapeChar[0];
					}
					if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
					{
						_pScintilla->execute(SCI_SETPROPERTY, (WPARAM)"userDefine.escapeChar", reinterpret_cast<LPARAM>(_pUserLang->_escapeChar)); 
						_pScintilla->styleChange();
					}
				}
			}
			// car LBN_SELCHANGE == CBN_SELCHANGE == 1
			else if ((HIWORD(wParam) == LBN_SELCHANGE) ||(HIWORD(wParam) == CBN_SELCHANGE))
            {
				if ((LOWORD(wParam) == IDC_ACTIVATED_SYMBOL_LIST) || (LOWORD(wParam) == IDC_AVAILABLE_SYMBOLS_LIST))
				{
					int idButton2Enable;
					int idButton2Disable;

					if (LOWORD(wParam) == IDC_AVAILABLE_SYMBOLS_LIST)
					{
						idButton2Enable = IDC_ADD_BUTTON;
						idButton2Disable = IDC_REMOVE_BUTTON;
					}
					else
					{
						idButton2Enable = IDC_REMOVE_BUTTON;
						idButton2Disable = IDC_ADD_BUTTON;
					}

					int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), LB_GETCURSEL, 0, 0);
					if (i != LB_ERR)
					{
						::EnableWindow(::GetDlgItem(_hSelf, idButton2Enable), TRUE);
						int idListbox2Disable = (LOWORD(wParam)== IDC_AVAILABLE_SYMBOLS_LIST)?IDC_ACTIVATED_SYMBOL_LIST:IDC_AVAILABLE_SYMBOLS_LIST;
						::SendDlgItemMessage(_hSelf, idListbox2Disable, LB_SETCURSEL, (WPARAM)-1, 0);
						::EnableWindow(::GetDlgItem(_hSelf, idButton2Disable), FALSE);
					}
					return TRUE;
				}
				else if ((LOWORD(wParam) == IDC_SYMBOL_BO2_COMBO) || (LOWORD(wParam) == IDC_SYMBOL_BC2_COMBO) ||
					(LOWORD(wParam) == IDC_SYMBOL_BO3_COMBO) || (LOWORD(wParam) == IDC_SYMBOL_BC3_COMBO))
				{
					TCHAR charStr[5] = TEXT("");
					int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
					::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETLBTEXT, i, (LPARAM)charStr);
					int symbIndex;

					if (LOWORD(wParam) == IDC_SYMBOL_BO2_COMBO)
						symbIndex = 0;
					else if (LOWORD(wParam) == IDC_SYMBOL_BO3_COMBO)
						symbIndex = 1;
					else if (LOWORD(wParam) == IDC_SYMBOL_BC2_COMBO)
						symbIndex = 3;
					else // (LOWORD(wParam) == IDC_SYMBOL_BC3_COMBO)
						symbIndex = 4;

					TCHAR *delims = _pUserLang->_keywordLists[KWL_DELIM_INDEX];
					delims[symbIndex] = charStr[0]?charStr[0]:'0';

					if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
							_pScintilla->styleChange();
					return TRUE;
				}
				else
					return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
			}
		}
		default :
			return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}

void SymbolsStyleDialog::undeleteChar()
{
	if ((::SendDlgItemMessage(_hSelf, IDC_HAS_ESCAPE, BM_GETCHECK,0,0) == BST_CHECKED) && 
	 (!::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_GETTEXTLENGTH, 0, 0)))
	{
		if (_pUserLang->_escapeChar[0])
			::SendDlgItemMessage(_hSelf, IDC_ESCAPE_CHAR, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(&_pUserLang->_escapeChar[0]));
		else
			::SendDlgItemMessage(_hSelf, IDC_HAS_ESCAPE, BM_SETCHECK, BST_UNCHECKED, 0);		
 	}
}

int SymbolsStyleDialog::getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const 
{
	switch (ctrlID)
    {
		case IDC_SYMBOL_BOLD_CHECK : 
		{
			fontStyleMask = FONTSTYLE_BOLD;
			return STYLE_OPERATOR_INDEX;
		}
		case IDC_SYMBOL_ITALIC_CHECK :
		{
			fontStyleMask = FONTSTYLE_ITALIC;
			return STYLE_OPERATOR_INDEX;
		}
		case IDC_SYMBOL_UNDERLINE_CHECK :
		{
			fontStyleMask = FONTSTYLE_UNDERLINE;
			return STYLE_OPERATOR_INDEX;
		}
		
		case IDC_SYMBOL_BOLD2_CHECK : 
		{
			fontStyleMask = FONTSTYLE_BOLD;
			return STYLE_DELIM2_INDEX;
		}
		case IDC_SYMBOL_ITALIC2_CHECK :
		{
			fontStyleMask = FONTSTYLE_ITALIC;
			return STYLE_DELIM2_INDEX;
		}
		case IDC_SYMBOL_UNDERLINE2_CHECK :
		{
			fontStyleMask = FONTSTYLE_UNDERLINE;
			return STYLE_DELIM2_INDEX;
		}
		
		case IDC_SYMBOL_BOLD3_CHECK : 
		{
			fontStyleMask = FONTSTYLE_BOLD;
			return STYLE_DELIM3_INDEX;
		}
		case IDC_SYMBOL_ITALIC3_CHECK :
		{
			fontStyleMask = FONTSTYLE_ITALIC;
			return STYLE_DELIM3_INDEX;
		}
		case IDC_SYMBOL_UNDERLINE3_CHECK :
		{
			fontStyleMask = FONTSTYLE_UNDERLINE;
			return STYLE_DELIM3_INDEX;
		}
		
		default :
            return -1;
	}
}

TCHAR styleName[][32] = {TEXT("DEFAULT"), TEXT("FOLDEROPEN"), TEXT("FOLDERCLOSE"), TEXT("KEYWORD1"), TEXT("KEYWORD2"), TEXT("KEYWORD3"), TEXT("KEYWORD4"), TEXT("COMMENT"), TEXT("COMMENT LINE"), TEXT("NUMBER"), TEXT("OPERATOR"), TEXT("DELIMINER1"), TEXT("DELIMINER2"), TEXT("DELIMINER3")};


UserDefineDialog::UserDefineDialog(): SharedParametersDialog(), _status(UNDOCK), _yScrollPos(0), _prevHightVal(0), _isDirty(false)
{
	_pCurrentUserLang = new UserLangContainer();

    // @REF #01 NE CHANGER PAS D'ORDRE !!!
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_IDENTIFIER, styleName[0]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_BLOCK_OPERATOR_OPEN, styleName[1]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_BLOCK_OPERATOR_CLOSE, styleName[2]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_WORD1, styleName[3]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_WORD2, styleName[4]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_WORD3, styleName[5]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_WORD4, styleName[6]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_COMMENT, styleName[7]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_COMMENTLINE, styleName[8]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_NUMBER, styleName[9]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_OPERATOR, styleName[10]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_DELIMITER1, styleName[11]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_DELIMITER2, styleName[12]);
	_pCurrentUserLang->_styleArray.addStyler(SCE_USER_DELIMITER3, styleName[13]);
}

UserDefineDialog::~UserDefineDialog()
{
	delete _pCurrentUserLang;
}

void UserDefineDialog::reloadLangCombo()
{
    NppParameters *pNppParam = NppParameters::getInstance();
    ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_RESETCONTENT, 0, 0);
	::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_ADDSTRING, 0, (LPARAM)TEXT("User Define Language"));
	for (int i = 0 ; i < pNppParam->getNbUserLang() ; i++)
	{
		UserLangContainer & userLangContainer = pNppParam->getULCFromIndex(i);
		::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_ADDSTRING, 0, (LPARAM)userLangContainer.getName());
	}
}

void UserDefineDialog::changeStyle()
{
    _status = !_status;
    ::SetDlgItemText(_hSelf, IDC_DOCK_BUTTON, (_status == DOCK)?TEXT("Undock"):TEXT("Dock"));

    long style = ::GetWindowLongPtr(_hSelf, GWL_STYLE);
    if (!style)
        ::MessageBox(NULL, TEXT("GetWindowLongPtr failed in UserDefineDialog::changeStyle()"), TEXT(""), MB_OK);

    style = (_status == DOCK)?
        ((style & ~WS_POPUP) & ~DS_MODALFRAME & ~WS_CAPTION) | WS_CHILD :
        (style & ~WS_CHILD) | WS_POPUP | DS_MODALFRAME | WS_CAPTION;

    long result = ::SetWindowLongPtr(_hSelf, GWL_STYLE, style);
    if (!result)
        ::MessageBox(NULL, TEXT("SetWindowLongPtr failed in UserDefineDialog::changeStyle()"), TEXT(""), MB_OK);    

    if (_status == DOCK)
        getActualPosSize();
    else
        restorePosSize();

    ::SetParent(_hSelf, (_status == DOCK)?_hParent:NULL);
}

void UserDefineDialog::enableLangAndControlsBy(int index)
{
	_pUserLang = (index == 0)?_pCurrentUserLang:&((NppParameters::getInstance())->getULCFromIndex(index - 1));
	if (index != 0)
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EXT_EDIT), _pUserLang->_ext.c_str());

	::ShowWindow(::GetDlgItem(_hSelf, IDC_EXT_STATIC), (index == 0)?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EXT_EDIT), (index == 0)?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RENAME_BUTTON), (index == 0)?SW_HIDE:SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REMOVELANG_BUTTON), (index == 0)?SW_HIDE:SW_SHOW);
}

void UserDefineDialog::updateDlg() 
{
	if (!_isDirty)
	{
		int i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
		if (i > 0)
			_isDirty = true;
	}
	::SendDlgItemMessage(_hSelf, IDC_LANGNAME_IGNORECASE_CHECK, BM_SETCHECK, _pUserLang->_isCaseIgnored, 0);
	_folderStyleDlg.updateDlg();
	_keyWordsStyleDlg.updateDlg();
	_commentStyleDlg.updateDlg();
	_symbolsStyleDlg.updateDlg();
}

BOOL CALLBACK UserDefineDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	switch (message) 
	{
        case WM_INITDIALOG :
        {
			_ctrlTab.init(_hInst, _hSelf, false);
			_ctrlTab.setFont(TEXT("Tahoma"), 13);

			_folderStyleDlg.init(_hInst, _hSelf);
			_folderStyleDlg.create(IDD_FOLDER_STYLE_DLG);
			_folderStyleDlg.display();

			_keyWordsStyleDlg.init(_hInst, _hSelf);
			_keyWordsStyleDlg.create(IDD_KEYWORD_STYLE_DLG);
			_keyWordsStyleDlg.display(false);

			_commentStyleDlg.init(_hInst, _hSelf);
			_commentStyleDlg.create(IDD_COMMENT_STYLE_DLG);
			_commentStyleDlg.display(false);

			_symbolsStyleDlg.init(_hInst, _hSelf);
			_symbolsStyleDlg.create(IDD_SYMBOL_STYLE_DLG);
			_symbolsStyleDlg.display(false);

			_wVector.push_back(DlgInfo(&_folderStyleDlg, TEXT("Folder && Default")));
			_wVector.push_back(DlgInfo(&_keyWordsStyleDlg, TEXT("Keywords Lists")));
			_wVector.push_back(DlgInfo(&_commentStyleDlg, TEXT("Comment && Number")));
			_wVector.push_back(DlgInfo(&_symbolsStyleDlg, TEXT("Operators")));

			_ctrlTab.createTabs(_wVector);
			_ctrlTab.display();

			RECT arc;
			::GetWindowRect(::GetDlgItem(_hSelf, IDC_IMPORT_BUTTON), &arc);

			POINT p;
			p.x = arc.left;
			p.y = arc.bottom;
			::ScreenToClient(_hSelf, &p);

			RECT rc;
			getClientRect(rc);
			rc.top = p.y + 10;	
			rc.bottom -= 100;
			_ctrlTab.reSizeTo(rc);

			_folderStyleDlg.reSizeTo(rc);
			_keyWordsStyleDlg.reSizeTo(rc);
			_commentStyleDlg.reSizeTo(rc);
			_symbolsStyleDlg.reSizeTo(rc);

			reloadLangCombo();
            ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, 0, 0);

			enableLangAndControlsBy(0);

			_pUserLang = _pCurrentUserLang;

			if (pNppParam->isTransparentAvailable())
			{
				::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_TRANSPARENT_CHECK), SW_SHOW);
				::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), SW_SHOW);
				
				::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
				::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 150);
				if (!(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, BM_GETCHECK, 0, 0)))
					::EnableWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), FALSE);
			}
			SCROLLINFO si;
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE; //| SIF_PAGE; 
			si.nMin   = 0; 
			si.nMax   = 0; 
			//si.nPage  = _currentHight; 
			//si.nPos = 0;
			::SetScrollInfo(_hSelf, SB_VERT, &si, TRUE);
	
			ETDTProc enableDlgTheme = (ETDTProc)pNppParam->getEnableThemeDlgTexture();
			if (enableDlgTheme)
			{
				//enableDlgTheme(_hSelf, ETDT_ENABLETAB);
			}

            return TRUE;
        }

		case WM_NOTIFY :		  
		{
			NMHDR *nmhdr = (NMHDR *)lParam;
			if (nmhdr->code == TCN_SELCHANGE)
			{
				if (nmhdr->hwndFrom == _ctrlTab.getHSelf())
				{
					_ctrlTab.clickedUpdate();
					return TRUE;
				}
			}
			break;
		}

		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER))
			{
				int percent = ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
				pNppParam->SetTransparent(_hSelf, percent/*HIWORD(wParam)*/);
			}
			return TRUE;
		}

		case WM_COMMAND : 
		{
            if (HIWORD(wParam) == EN_CHANGE)
            {
				TCHAR ext[extsLenMax];
				::SendDlgItemMessage(_hSelf, IDC_EXT_EDIT, WM_GETTEXT, extsLenMax, (LPARAM)ext);
				_pUserLang->_ext = ext;
                return TRUE;
            }
            else if (HIWORD(wParam) == CBN_SELCHANGE)
            {
				if (LOWORD(wParam) == IDC_LANGNAME_COMBO)
				{
					int i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
					enableLangAndControlsBy(i);
					updateDlg();
				}
                return TRUE;
            }
            else
            {
			    switch (wParam)
			    {
				    case IDC_DOCK_BUTTON :
                    {
						int msg = WM_UNDOCK_USERDEFINE_DLG;
						
						if (_status == UNDOCK)
						{
							if (pNppParam->isTransparentAvailable())
							{
								pNppParam->removeTransparent(_hSelf);
								::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_TRANSPARENT_CHECK), SW_HIDE);
								::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), SW_HIDE);
							}
							msg = WM_DOCK_USERDEFINE_DLG;
						}

                        changeStyle();

						if (_status == UNDOCK)
						{
							if (pNppParam->isTransparentAvailable())
							{
								bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_UD_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
								if (isChecked)
								{
									int percent = ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
									pNppParam->SetTransparent(_hSelf, percent);
								}
								::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_TRANSPARENT_CHECK), SW_SHOW);
								::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), SW_SHOW);
							}
						}
                        ::SendMessage(_hParent, msg, 0, 0);
					    return TRUE;
                    }
    				case IDCANCEL :
						::SendMessage(_hParent, WM_CLOSE_USERDEFINE_DLG, 0, 0);
					    display(false);
					    return TRUE;

				    case IDC_REMOVELANG_BUTTON :
                    {
						int result = ::MessageBox(_hSelf, TEXT("Are you sure?"), TEXT("Remove the current language"), MB_YESNO);
						if (result == IDYES)
						{
							int i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
							TCHAR langName[256];
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXT, i, (LPARAM)langName);

							//remove current language from combobox
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_DELETESTRING, i, 0);
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, i-1, 0);
							::SendMessage(_hSelf, WM_COMMAND, MAKELONG(IDC_LANGNAME_COMBO, CBN_SELCHANGE), (LPARAM)::GetDlgItem(_hSelf, IDC_LANGNAME_COMBO));

							//remove current language from userLangArray
							pNppParam->removeUserLang(i-1);

							//remove current language from langMenu
							HWND hNpp = ::GetParent(_hSelf);
							::RemoveMenu(::GetSubMenu((HMENU)::SendMessage(hNpp, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_LANGUAGE), IDM_LANG_USER + i, MF_BYCOMMAND);
							::DrawMenuBar(hNpp);
							::SendMessage(_hParent, WM_REMOVE_USERLANG, 0, (LPARAM)langName);
						}
					    return TRUE;
                    }
				    case IDC_RENAME_BUTTON :
                    {
						TCHAR langName[256];
						int i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXT, i, (LPARAM)langName);

						StringDlg strDlg;
						strDlg.init(_hInst, _hSelf, TEXT("Rename Current Language Name"), TEXT("Name : "), langName, langNameLenMax-1);

						TCHAR *newName = (TCHAR *)strDlg.doDialog();

						if (newName)
						{
							if (pNppParam->isExistingUserLangName(newName))
							{
								::MessageBox(_hSelf, TEXT("This name is used by another language,\rplease give another one."), TEXT("Err"), MB_OK);
								::PostMessage(_hSelf, WM_COMMAND, IDC_RENAME_BUTTON, 0);
								return TRUE;
							}
							//rename current language name in combobox
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_DELETESTRING, i, 0);
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_INSERTSTRING, i, (LPARAM)newName);
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, i, 0);
							
							//rename current language name in userLangArray
							UserLangContainer & userLangContainer = pNppParam->getULCFromIndex(i-1);
							userLangContainer._name = newName;

							//rename current language name in langMenu
							HWND hNpp = ::GetParent(_hSelf);
							::ModifyMenu(::GetSubMenu((HMENU)::SendMessage(hNpp, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_LANGUAGE), IDM_LANG_USER + i, MF_BYCOMMAND, IDM_LANG_USER + i, newName);
							::DrawMenuBar(hNpp);
							::SendMessage(_hParent, WM_RENAME_USERLANG, (WPARAM)newName, (LPARAM)langName);
						}

					    return TRUE;
                    }

					case IDC_ADDNEW_BUTTON :
					case IDC_SAVEAS_BUTTON :
                    {
						//TCHAR langName[256];
						int i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
						//::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXT, i, (LPARAM)langName);
						if (i == 0)
							wParam = IDC_ADDNEW_BUTTON;

						StringDlg strDlg;
						if (wParam == IDC_SAVEAS_BUTTON)
							strDlg.init(_hInst, _hSelf, TEXT("Save Current Language Name As..."), TEXT("Name : "), TEXT(""), langNameLenMax-1);
						else
							strDlg.init(_hInst, _hSelf, TEXT("Create New Language..."), TEXT("Name : "), TEXT(""), langNameLenMax-1);

						TCHAR *tmpName = (TCHAR *)strDlg.doDialog();
						//const TCHAR *newName = newNameString.c_str();

						if (tmpName && tmpName[0])
						{
							generic_string newNameString(tmpName);
							const TCHAR *newName = newNameString.c_str();

							if (pNppParam->isExistingUserLangName(newName))
							{
								::MessageBox(_hSelf, TEXT("This name is used by another language,\rplease give another one."), TEXT("Err"), MB_OK);
								::PostMessage(_hSelf, WM_COMMAND, IDC_RENAME_BUTTON, 0);
								return TRUE;
							}
							//add current language in userLangArray at the end as a new lang
							UserLangContainer & userLang = (wParam == IDC_SAVEAS_BUTTON)?pNppParam->getULCFromIndex(i-1):*_pCurrentUserLang;
							int newIndex = pNppParam->addUserLangToEnd(userLang, newName);

							//add new language name in combobox
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_ADDSTRING, 0, LPARAM(newName));
							::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, newIndex + 1, 0);
							::SendMessage(_hSelf, WM_COMMAND, MAKELONG(IDC_LANGNAME_COMBO, CBN_SELCHANGE), (LPARAM)::GetDlgItem(_hSelf, IDC_LANGNAME_COMBO));

							//add new language name in langMenu
							HWND hNpp = ::GetParent(_hSelf);
							::InsertMenu(::GetSubMenu((HMENU)::SendMessage(hNpp, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_LANGUAGE), IDM_LANG_USER + newIndex /*+ 1*/, MF_BYCOMMAND, IDM_LANG_USER + newIndex + 1, newName);
							::DrawMenuBar(hNpp);
						}

					    return TRUE;
                    }
					case IDC_IMPORT_BUTTON :
                    {
                        NppParameters *pNppParam = NppParameters::getInstance();

                        FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
		                fDlg.setExtFilter(TEXT("UDL"), TEXT(".xml"), NULL);
                        TCHAR *fn = fDlg.doOpenSingleFileDlg();
                        if (!fn) break;
                        generic_string sourceFile = fn;

                        bool isSuccessful = pNppParam->importUDLFromFile(sourceFile);
                        if (isSuccessful)
                        {
                            int i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
                            reloadLangCombo();
                            ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, i, 0);
                            _isDirty = true;
                            printStr(TEXT("Import successful."));
                        }
                        else
                        {
                            printStr(TEXT("Fail to import."));
                        }
                        break;
                    }

					case IDC_EXPORT_BUTTON :
                    {
                        NppParameters *pNppParam = NppParameters::getInstance();

                        FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
		                fDlg.setExtFilter(TEXT("UDL"), TEXT(".xml"), NULL);
                        TCHAR *fn = fDlg.doSaveDlg();
                        if (!fn) break;
                        generic_string fileName2save = fn;

                        int i2Export = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
                        
                        if (i2Export > 0)
                        {
                            bool isSuccessful = pNppParam->exportUDLToFile(i2Export - 1, fileName2save);
                            if (isSuccessful)
                            {
                                printStr(TEXT("Export successful"));
                            }
                            else
                            {
                                printStr(TEXT("Fail to export."));
                            }
                        }
                        break;
                    }

					case IDC_UD_TRANSPARENT_CHECK :
					{
						bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_UD_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
						if (isChecked)
						{
							int percent = ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0);
							pNppParam->SetTransparent(_hSelf, percent);
						}
						else
							pNppParam->removeTransparent(_hSelf);

						::EnableWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), isChecked);
						return TRUE;
					}

					case IDC_LANGNAME_IGNORECASE_CHECK :
						return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isCaseIgnored);

				    default :
					    break;
			    }
            }
			return FALSE;
		}

		case WM_DESTROY:
		{
			_folderStyleDlg.destroy();
			_keyWordsStyleDlg.destroy();
			_commentStyleDlg.destroy();
			_symbolsStyleDlg.destroy();

			_ctrlTab.destroy();
			return TRUE;
		}

		case WM_SIZE: 
		{
			int originalHight = _dlgPos.bottom; //- ((_status == DOCK)?_dlgPos.top:0);
			_currentHight = HIWORD (lParam);

			int diff = _currentHight - _prevHightVal;
			_prevHightVal = _currentHight;

			 int maxPos = originalHight - _currentHight;
			// Set the vertical scrolling range and page size
			SCROLLINFO si;
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_RANGE | SIF_PAGE; 
			si.nMin   = 0; 
			si.nMax   = (_status == UNDOCK)?0:originalHight; 
			si.nPage  = _currentHight; 
			//si.nPos = 0;
			::SetScrollInfo(_hSelf, SB_VERT, &si, TRUE);

			if ((_yScrollPos >= maxPos) && (_currentHight < originalHight))
			{
				//int nDelta = min(max(maxPos/10,5), maxPos - _yScrollPos);
				if (_yScrollPos > 0)
				{
					_yScrollPos -= diff;
					::SetScrollPos(_hSelf, SB_VERT, _yScrollPos, TRUE);
					::ScrollWindow(_hSelf, 0, diff, NULL, NULL);
				}
			}
			return TRUE; 
		}

		case WM_VSCROLL :
		{
			int originalHight = _dlgPos.bottom;
			int oldy = _yScrollPos;
			int maxPos = originalHight - _currentHight;

			switch (LOWORD (wParam))
			{
				// user clicked the top arrow
				case SB_LINEUP:
					if (_yScrollPos <= 0)
						return FALSE;
					_yScrollPos = 0;
					break;

				// user clicked the bottom arrow
				case SB_LINEDOWN:
					if (_yScrollPos >= maxPos)
						return FALSE;
					_yScrollPos = maxPos;
					break;

				case SB_PAGEDOWN:
					if (_yScrollPos >= maxPos)
						return FALSE;
					_yScrollPos = maxPos;
					break;

				case SB_PAGEUP:
					if (_yScrollPos <= 0)
						return FALSE;
					_yScrollPos = 0;
					break;

				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					_yScrollPos = (int)HIWORD(wParam);
					break;

				default :
					return FALSE;
			}
			::SetScrollPos(_hSelf, SB_VERT, _yScrollPos, TRUE);
			::ScrollWindow(_hSelf, 0, oldy-_yScrollPos, NULL, NULL);
		}
		case NPPM_MODELESSDIALOG :
			return ::SendMessage(_hParent, NPPM_MODELESSDIALOG, wParam, lParam);
    }
	
	return FALSE;
}

BOOL CALLBACK StringDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{

	switch (Message)
	{
		case WM_INITDIALOG :
		{
			::SetWindowText(_hSelf, _title.c_str());
			::SetDlgItemText(_hSelf, IDC_STRING_STATIC, _static.c_str());
			::SetDlgItemText(_hSelf, IDC_STRING_EDIT, _textValue.c_str());
			if (_txtLen)
				::SendDlgItemMessage(_hSelf, IDC_STRING_EDIT, EM_SETLIMITTEXT, _txtLen, 0);

			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDOK :
				{
					TCHAR tmpName[langNameLenMax];
					tmpName[0] = '\0';
					::GetDlgItemText(_hSelf, IDC_STRING_EDIT, (LPTSTR)tmpName, langNameLenMax);
					_textValue = tmpName;
					::EndDialog(_hSelf, int(_textValue.c_str()));
					return TRUE;
				}

				case IDCANCEL :
					::EndDialog(_hSelf, 0);
					return TRUE;

				default:
					return FALSE;
			}
		}
		default :
			return FALSE;
	}
}
