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


#include "localization.h"
#include "UserDefineDialog.h"
#include "ScintillaEditView.h"
#include "Parameters.h"
#include "resource.h"
#include "Notepad_plus_msgs.h"
#include "FileDialog.h"
#include "Common.h"

using namespace std;

UserLangContainer * SharedParametersDialog::_pUserLang = NULL;
ScintillaEditView * SharedParametersDialog::_pScintilla = NULL;

GlobalMappers & globalMappper()
{
    // getting global object as reference to static ensures that only one object is created
    // even if called from different code units
    static GlobalMappers gm;
    return gm;
}

bool SharedParametersDialog::setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set)
{
    bool2set = (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, id), BM_GETCHECK, 0, 0));

    if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
        _pScintilla->styleChange();

    return TRUE;
}

INT_PTR CALLBACK SharedParametersDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (Message)
    {
        case WM_INITDIALOG :
        {
            // initControls();
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
            return FALSE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK FolderStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_INITDIALOG :
        {
            _pageLink.init(_hInst, _hSelf);
            _pageLink.create(::GetDlgItem(_hSelf, IDC_WEB_HELP_LINK), TEXT("http://udl20.weebly.com/"));
            return TRUE;
        }

        case WM_COMMAND :
        {
            switch (wParam)
            {
                case IDC_FOLDER_FOLD_COMPACT :
                {
                    return setPropertyByCheck(_hSelf, wParam, _pUserLang->_foldCompact);
                }
                case IDC_FOLDER_IN_CODE1_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_FOLDER_IN_CODE1, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_FOLDER_IN_CODE2_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_FOLDER_IN_CODE2, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_FOLDER_IN_COMMENT_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_FOLDER_IN_COMMENT, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DEFAULT_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DEFAULT, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                default :
                    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
            }
        }
        case WM_DESTROY:
        {
            _pageLink.destroy();
            return TRUE;
        }
        default :
            return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
    }
}

void FolderStyleDialog::setKeywords2List(int id)
{
    switch (id)
    {
        case IDC_FOLDER_IN_CODE1_OPEN_EDIT :     
        case IDC_FOLDER_IN_CODE1_MIDDLE_EDIT :   
        case IDC_FOLDER_IN_CODE1_CLOSE_EDIT :    
        case IDC_FOLDER_IN_CODE2_OPEN_EDIT :     
        case IDC_FOLDER_IN_CODE2_MIDDLE_EDIT :   
        case IDC_FOLDER_IN_CODE2_CLOSE_EDIT :    
        case IDC_FOLDER_IN_COMMENT_OPEN_EDIT :   
        case IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT : 
        case IDC_FOLDER_IN_COMMENT_CLOSE_EDIT :  
            ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[globalMappper().dialogMapper[id]], max_char);
            break;
    }
}

void FolderStyleDialog::updateDlg()
{
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_FOLD_COMPACT,           BM_SETCHECK, _pUserLang->_foldCompact, 0);

    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE1_OPEN_EDIT,     WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE1_MIDDLE_EDIT,   WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE1_CLOSE_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE2_OPEN_EDIT,     WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE2_MIDDLE_EDIT,   WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE2_CLOSE_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_COMMENT_OPEN_EDIT,   WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE]));
    ::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_COMMENT_CLOSE_EDIT,  WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE]));
}

void FolderStyleDialog::convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR *prefix) const
{
    bool inGroup = false;
    int index = lstrlen(dest);
    if (index > 0)
        dest[index++] = ' ';
    dest[index++] = prefix[0];
    dest[index++] = prefix[1];

    for (size_t i = 0, len = lstrlen(toConvert); i < len ; ++i)
    {
        if (i == 0 && toConvert[i] == '(' && toConvert[i+1] == '(')
        {
            inGroup = true;
        }
        else if (toConvert[i] == ' ' && toConvert[i+1] == '(' && toConvert[i+2] == '(')
        {
            inGroup = true;
            dest[index++] = ' ';
            dest[index++] = prefix[0];
            dest[index++] = prefix[1];
            ++i;    // skip space
        }
        if (inGroup && toConvert[i-1] == ')' && toConvert[i-2] == ')')
        {
            inGroup = false;
        }

        if (toConvert[i] == ' ')
        {
            if (toConvert[i+1] != ' ' && toConvert[i+1] != '\0')
            {
                dest[index++] = ' ';
                if (!inGroup)
                {
                    dest[index++] = prefix[0];
                    dest[index++] = prefix[1];
                }
            }
        }
        else
        {
            dest[index++] = toConvert[i];
        }
    }
    dest[index] = '\0';
}

void FolderStyleDialog::retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const
{
    int j = 0;
    bool begin2Copy = false;

    for (size_t i = 0, len = lstrlen(toRetrieve); i < len ; ++i)
    {
        if ((i == 0 || (toRetrieve[i-1] == ' ')) && (toRetrieve[i] == prefix[0] && toRetrieve[i+1] == prefix[1]))
        {
            if (j > 0)
                dest[j++] = ' ';

            begin2Copy = true;
            ++i;
            continue;
        }
        else if (((toRetrieve[i] == ' ') && begin2Copy == true))
        {
            begin2Copy = false;
        }

        if (begin2Copy)
            dest[j++] = toRetrieve[i];
    }
    dest[j++] = '\0';
}

INT_PTR CALLBACK KeyWordsStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_INITDIALOG :
        {
            // extend Keyword edit boxes to hold 128k of TCHARs
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD1_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD2_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD3_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD4_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD5_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD6_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD7_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
            ::SendMessage(::GetDlgItem(_hSelf,IDC_KEYWORD8_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);

            return TRUE;
        }

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

                case IDC_KEYWORD5_PREFIX_CHECK :
                    return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[4]);

                case IDC_KEYWORD6_PREFIX_CHECK :
                    return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[5]);

                case IDC_KEYWORD7_PREFIX_CHECK :
                    return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[6]);

                case IDC_KEYWORD8_PREFIX_CHECK :
                    return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[7]);

                case IDC_KEYWORD1_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD1, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD2_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD2, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD3_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD3, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD4_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD4, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD5_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD5, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD6_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD6, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD7_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD7, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_KEYWORD8_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD8, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                default :
                    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
            }
        }
        default :
            return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
    }
}

void KeyWordsStyleDialog::setKeywords2List(int id)
{
    switch (id)
    {
        case IDC_KEYWORD1_EDIT :
        case IDC_KEYWORD2_EDIT :
        case IDC_KEYWORD3_EDIT :
        case IDC_KEYWORD4_EDIT :
        case IDC_KEYWORD5_EDIT :
        case IDC_KEYWORD6_EDIT :
        case IDC_KEYWORD7_EDIT :
        case IDC_KEYWORD8_EDIT :
            ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[globalMappper().dialogMapper[id]], max_char);
    }
}

void KeyWordsStyleDialog::updateDlg()
{
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD1_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS1]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD2_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS2]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD3_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS3]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD4_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS4]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD5_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS5]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD6_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS6]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD7_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS7]));
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD8_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS8]));

    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD1_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[0], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD2_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[1], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD3_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[2], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD4_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[3], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD5_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[4], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD6_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[5], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD7_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[6], 0);
    ::SendDlgItemMessage(_hSelf, IDC_KEYWORD8_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[7], 0);
}

INT_PTR CALLBACK CommentStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_ACTIVATE :
        case WM_SHOWWINDOW :
        {
            ::SendDlgItemMessage(_hSelf, IDC_ALLOW_ANYWHERE,    BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_NONE, 0);
            ::SendDlgItemMessage(_hSelf, IDC_FORCE_AT_BOL,      BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_BOL,  0);
            ::SendDlgItemMessage(_hSelf, IDC_ALLOW_WHITESPACE,  BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_WSP,  0);

            ::SendDlgItemMessage(_hSelf, IDC_DOT_RADIO,         BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_DOT,   0);
            ::SendDlgItemMessage(_hSelf, IDC_COMMA_RADIO,       BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_COMMA, 0);
            ::SendDlgItemMessage(_hSelf, IDC_BOTH_RADIO,        BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_BOTH,  0);

            return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
        }

        case WM_COMMAND :
        {
            switch (wParam)
            {
                case IDC_FOLDING_OF_COMMENTS :
                {
                    return setPropertyByCheck(_hSelf, wParam, _pUserLang->_allowFoldOfComments);
                }
                case IDC_ALLOW_ANYWHERE :
                case IDC_FORCE_AT_BOL :
                case IDC_ALLOW_WHITESPACE :
                {
                    if (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_ALLOW_ANYWHERE), BM_GETCHECK, 0, 0))
                        _pUserLang->_forcePureLC = PURE_LC_NONE;
                    else if (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_FORCE_AT_BOL), BM_GETCHECK, 0, 0))
                        _pUserLang->_forcePureLC = PURE_LC_BOL;
                    else if (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_ALLOW_WHITESPACE), BM_GETCHECK, 0, 0))
                        _pUserLang->_forcePureLC = PURE_LC_WSP;

                    if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
                        _pScintilla->styleChange();

                    return TRUE;
                }
                case IDC_DOT_RADIO :
                case IDC_COMMA_RADIO :
                case IDC_BOTH_RADIO :
                {
                    if (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_DOT_RADIO), BM_GETCHECK, 0, 0))
                        _pUserLang->_decimalSeparator = DECSEP_DOT;
                    else if (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_COMMA_RADIO), BM_GETCHECK, 0, 0))
                        _pUserLang->_decimalSeparator = DECSEP_COMMA;
                    else if (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_BOTH_RADIO), BM_GETCHECK, 0, 0))
                        _pUserLang->_decimalSeparator = DECSEP_BOTH;

                    if (_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
                        _pScintilla->styleChange();

                    return TRUE;
                }
                case IDC_COMMENTLINE_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_COMMENTLINE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_COMMENT_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_COMMENT);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_NUMBER_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_NUMBER, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                default :
                    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
            }
        }
        default :
            return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
    }
}

void CommentStyleDialog::setKeywords2List(int id)
{
    int index = 0;
    switch (id)
    {
        case IDC_NUMBER_PREFIX1_EDIT :
        case IDC_NUMBER_PREFIX2_EDIT :
        case IDC_NUMBER_EXTRAS1_EDIT :
        case IDC_NUMBER_EXTRAS2_EDIT :
        case IDC_NUMBER_SUFFIX1_EDIT :
        case IDC_NUMBER_SUFFIX2_EDIT :
        case IDC_NUMBER_RANGE_EDIT :  
        {
            ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[globalMappper().dialogMapper[id]], max_char);
            break;
        }

        case IDC_COMMENT_OPEN_EDIT :
        case IDC_COMMENT_CLOSE_EDIT :
        case IDC_COMMENTLINE_OPEN_EDIT :
        case IDC_COMMENTLINE_CONTINUE_EDIT :
        case IDC_COMMENTLINE_CLOSE_EDIT :
            index = SCE_USER_KWLIST_COMMENTS;
            break;
        default : index = -1;
    }
    if (index != -1)
    {
        TCHAR newList[max_char] = TEXT("");
        TCHAR buffer[max_char] = TEXT("");
        TCHAR intBuffer[10] = {'0', 0};

        const int list[] = {
            IDC_COMMENTLINE_OPEN_EDIT,
            IDC_COMMENTLINE_CONTINUE_EDIT,
            IDC_COMMENTLINE_CLOSE_EDIT,
            IDC_COMMENT_OPEN_EDIT,
            IDC_COMMENT_CLOSE_EDIT
        };

        for (int i=0; i<sizeof(list)/sizeof(int); ++i)
        {
            generic_itoa(i, intBuffer+1, 10);
            ::GetDlgItemText(_hSelf, list[i], buffer, max_char);
            convertTo(newList, buffer, intBuffer);
        }

        lstrcpy(_pUserLang->_keywordLists[index], newList);
    }
}

void CommentStyleDialog::convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR *prefix) const
{
    bool inGroup = false;
    int index = lstrlen(dest);
    if (index > 0)
        dest[index++] = ' ';
    dest[index++] = prefix[0];
    dest[index++] = prefix[1];

    for (size_t i = 0, len = lstrlen(toConvert); i < len ; ++i)
    {
        if (i == 0 && toConvert[i] == '(' && toConvert[i+1] == '(')
        {
            inGroup = true;
        }
        else if (toConvert[i] == ' ' && toConvert[i+1] == '(' && toConvert[i+2] == '(')
        {
            inGroup = true;
            dest[index++] = ' ';
            dest[index++] = prefix[0];
            dest[index++] = prefix[1];
            ++i;    // skip space
        }
        if (inGroup && toConvert[i-1] == ')' && toConvert[i-2] == ')')
        {
            inGroup = false;
        }

        if (toConvert[i] == ' ')
        {
            if (toConvert[i+1] != ' ' && toConvert[i+1] != '\0')
            {
                dest[index++] = ' ';
                if (!inGroup)
                {
                    dest[index++] = prefix[0];
                    dest[index++] = prefix[1];
                }
            }
        }
        else
        {
            dest[index++] = toConvert[i];
        }
    }
    dest[index] = '\0';
}

void CommentStyleDialog::retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const
{
    int j = 0;
    bool begin2Copy = false;
    bool inGroup = false;

    for (size_t i = 0, len = lstrlen(toRetrieve); i < len ; ++i)
    {
        if ((i == 0 || (toRetrieve[i-1] == ' ')) && (toRetrieve[i] == prefix[0] && toRetrieve[i+1] == prefix[1]))
        {
            if (j > 0)
                dest[j++] = ' ';

            begin2Copy = true;
            ++i;
            continue;
        }
        if (toRetrieve[i] == '(' && toRetrieve[i+1] == '(' && inGroup == false && begin2Copy == true)
        {
            inGroup = true;
        }
        if (toRetrieve[i] != ')' && toRetrieve[i-1] == ')' && toRetrieve[i-2] == ')' && inGroup == true)
        {
            inGroup = false;
        }
        if (toRetrieve[i] == ' ' && begin2Copy == true)
        {
            begin2Copy = false;
        }

        if (begin2Copy || inGroup)
            dest[j++] = toRetrieve[i];
    }
    dest[j++] = '\0';
}

void CommentStyleDialog::updateDlg()
{
    TCHAR buffer[max_char] = TEXT("");
    TCHAR intBuffer[10] = {'0', 0};

    const int list[] = {
        IDC_COMMENTLINE_OPEN_EDIT,
        IDC_COMMENTLINE_CONTINUE_EDIT,
        IDC_COMMENTLINE_CLOSE_EDIT,
        IDC_COMMENT_OPEN_EDIT,
        IDC_COMMENT_CLOSE_EDIT
    };

    for (int i=0; i<sizeof(list)/sizeof(int); ++i)
    {
        generic_itoa(i, intBuffer+1, 10);
        retrieve(buffer, _pUserLang->_keywordLists[SCE_USER_KWLIST_COMMENTS], intBuffer);
        ::SendDlgItemMessage(_hSelf, list[i], WM_SETTEXT, 0, (LPARAM)buffer);
    }

    ::SendDlgItemMessage(_hSelf, IDC_FOLDING_OF_COMMENTS,   BM_SETCHECK, _pUserLang->_allowFoldOfComments,    0);

    ::SendDlgItemMessage(_hSelf, IDC_ALLOW_ANYWHERE,        BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_NONE, 0);
    ::SendDlgItemMessage(_hSelf, IDC_FORCE_AT_BOL,          BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_BOL,  0);
    ::SendDlgItemMessage(_hSelf, IDC_ALLOW_WHITESPACE,      BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_WSP,  0);

    ::SendDlgItemMessage(_hSelf, IDC_DOT_RADIO,             BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_DOT, 0);
    ::SendDlgItemMessage(_hSelf, IDC_COMMA_RADIO,           BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_COMMA,  0);
    ::SendDlgItemMessage(_hSelf, IDC_BOTH_RADIO,            BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_BOTH,  0);

    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_PREFIX1_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_PREFIX1]));
    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_PREFIX2_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_PREFIX2]));
    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_EXTRAS1_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_EXTRAS1]));
    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_EXTRAS2_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_EXTRAS2]));
    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_SUFFIX1_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_SUFFIX1]));
    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_SUFFIX2_EDIT,    WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_SUFFIX2]));
    ::SendDlgItemMessage(_hSelf, IDC_NUMBER_RANGE_EDIT,      WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_RANGE]));
}

void SymbolsStyleDialog::updateDlg()
{
    TCHAR buffer[max_char] = TEXT("");
    const int list[] = {
        IDC_DELIMITER1_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER1_ESCAPE_EDIT,
        IDC_DELIMITER1_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER2_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER2_ESCAPE_EDIT,
        IDC_DELIMITER2_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER3_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER3_ESCAPE_EDIT,
        IDC_DELIMITER3_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER4_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER4_ESCAPE_EDIT,
        IDC_DELIMITER4_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER5_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER5_ESCAPE_EDIT,
        IDC_DELIMITER5_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER6_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER6_ESCAPE_EDIT,
        IDC_DELIMITER6_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER7_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER7_ESCAPE_EDIT,
        IDC_DELIMITER7_BOUNDARYCLOSE_EDIT,
        IDC_DELIMITER8_BOUNDARYOPEN_EDIT,
        IDC_DELIMITER8_ESCAPE_EDIT,
        IDC_DELIMITER8_BOUNDARYCLOSE_EDIT
    };
    TCHAR intBuffer[10] = {'0', 0};

    for (int i=0; i<sizeof(list)/sizeof(int); ++i)
    {
        if (i < 10)
            generic_itoa(i, intBuffer+1, 10);
        else
            generic_itoa(i, intBuffer, 10);

        retrieve(buffer, _pUserLang->_keywordLists[SCE_USER_KWLIST_DELIMITERS], intBuffer);
        ::SendDlgItemMessage(_hSelf, list[i], WM_SETTEXT, 0, (LPARAM)buffer);
    }

    ::SendDlgItemMessage(_hSelf, IDC_OPERATOR1_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS1]));
    ::SendDlgItemMessage(_hSelf, IDC_OPERATOR2_EDIT, WM_SETTEXT, 0, (LPARAM)(_pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS2]));
}

INT_PTR CALLBACK SymbolsStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_COMMAND :
        {
            switch (wParam)
            {
                case IDC_OPERATOR_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_OPERATOR, SCE_USER_MASK_NESTING_NONE);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER1_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER1);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER2_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER2);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER3_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER3);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER4_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER4);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER5_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER5);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER6_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER6);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER7_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER7);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                case IDC_DELIMITER8_STYLER :
                {
                    StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER8);
                    stylerDlg.doDialog();
                    return TRUE;
                }
                default :
                    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
            }
        }
        default :
            return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
    }
}

void SymbolsStyleDialog::convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR *prefix) const
{
    bool inGroup = false;
    int index = lstrlen(dest);
    if (index > 0)
        dest[index++] = ' ';
    dest[index++] = prefix[0];
    dest[index++] = prefix[1];

    for (size_t i = 0, len = lstrlen(toConvert); i < len ; ++i)
    {
        if (i == 0 && toConvert[i] == '(' && toConvert[i+1] == '(')
        {
            inGroup = true;
        }
        else if (toConvert[i] == ' ' && toConvert[i+1] == '(' && toConvert[i+2] == '(')
        {
            inGroup = true;
            dest[index++] = ' ';
            dest[index++] = prefix[0];
            dest[index++] = prefix[1];
            ++i;    // skip space
        }
        if (inGroup && toConvert[i-1] == ')' && toConvert[i-2] == ')')
        {
            inGroup = false;
        }

        if (toConvert[i] == ' ')
        {
            if (toConvert[i+1] != ' ' && toConvert[i+1] != '\0')
            {
                dest[index++] = ' ';
                if (!inGroup)
                {
                    dest[index++] = prefix[0];
                    dest[index++] = prefix[1];
                }
            }
        }
        else
        {
            dest[index++] = toConvert[i];
        }
    }
    dest[index] = '\0';
}

void SymbolsStyleDialog::retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const
{
    int j = 0;
    bool begin2Copy = false;
    bool inGroup = false;

    for (size_t i = 0, len = lstrlen(toRetrieve); i < len ; ++i)
    {
        if ((i == 0 || (toRetrieve[i-1] == ' ')) && (toRetrieve[i] == prefix[0] && toRetrieve[i+1] == prefix[1]))
        {
            if (j > 0)
                dest[j++] = ' ';

            begin2Copy = true;
            ++i;
            continue;
        }
        if (toRetrieve[i] == '(' && toRetrieve[i+1] == '(' && inGroup == false && begin2Copy == true)
        {
            inGroup = true;
        }
        if (toRetrieve[i] != ')' && toRetrieve[i-1] == ')' && toRetrieve[i-2] == ')' && inGroup == true)
        {
            inGroup = false;
        }
        if (toRetrieve[i] == ' ' && begin2Copy == true)
        {
            begin2Copy = false;
        }

        if (begin2Copy || inGroup)
            dest[j++] = toRetrieve[i];
    }
    dest[j++] = '\0';
}

void SymbolsStyleDialog::setKeywords2List(int id)
{
    switch (id)
    {
        case IDC_OPERATOR1_EDIT :
            ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS1], max_char);
            break;
        case IDC_OPERATOR2_EDIT :
            ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS2], max_char);
            break;

        case IDC_DELIMITER1_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER1_ESCAPE_EDIT :
        case IDC_DELIMITER1_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER2_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER2_ESCAPE_EDIT :
        case IDC_DELIMITER2_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER3_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER3_ESCAPE_EDIT :
        case IDC_DELIMITER3_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER4_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER4_ESCAPE_EDIT :
        case IDC_DELIMITER4_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER5_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER5_ESCAPE_EDIT :
        case IDC_DELIMITER5_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER6_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER6_ESCAPE_EDIT :
        case IDC_DELIMITER6_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER7_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER7_ESCAPE_EDIT :
        case IDC_DELIMITER7_BOUNDARYCLOSE_EDIT :
        case IDC_DELIMITER8_BOUNDARYOPEN_EDIT :
        case IDC_DELIMITER8_ESCAPE_EDIT :
        case IDC_DELIMITER8_BOUNDARYCLOSE_EDIT :
        {
            TCHAR newList[max_char] = TEXT("");
            TCHAR buffer[max_char] = TEXT("");
            TCHAR intBuffer[10] = {'0', 0};

            const int list[] = {
                IDC_DELIMITER1_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER1_ESCAPE_EDIT,
                IDC_DELIMITER1_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER2_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER2_ESCAPE_EDIT,
                IDC_DELIMITER2_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER3_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER3_ESCAPE_EDIT,
                IDC_DELIMITER3_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER4_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER4_ESCAPE_EDIT,
                IDC_DELIMITER4_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER5_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER5_ESCAPE_EDIT,
                IDC_DELIMITER5_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER6_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER6_ESCAPE_EDIT,
                IDC_DELIMITER6_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER7_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER7_ESCAPE_EDIT,
                IDC_DELIMITER7_BOUNDARYCLOSE_EDIT,
                IDC_DELIMITER8_BOUNDARYOPEN_EDIT,
                IDC_DELIMITER8_ESCAPE_EDIT,
                IDC_DELIMITER8_BOUNDARYCLOSE_EDIT
            };

            for (int i=0; i<sizeof(list)/sizeof(int); ++i)
            {
                if (i < 10)
                    generic_itoa(i, intBuffer+1, 10);
                else
                    generic_itoa(i, intBuffer, 10);

                int dd = list[i];
                ::GetDlgItemText(_hSelf, dd, buffer, max_char);
                convertTo(newList, buffer, intBuffer);
            }

            lstrcpy(_pUserLang->_keywordLists[SCE_USER_KWLIST_DELIMITERS], newList);
            break;
        }
        default :
            break;
    }
}

UserDefineDialog::UserDefineDialog(): SharedParametersDialog(), _status(UNDOCK), _yScrollPos(0), _prevHightVal(0), _isDirty(false)
{
    _pCurrentUserLang = new UserLangContainer();

    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DEFAULT,              globalMappper().styleNameMapper[SCE_USER_STYLE_DEFAULT].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_COMMENT,              globalMappper().styleNameMapper[SCE_USER_STYLE_COMMENT].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_COMMENTLINE,          globalMappper().styleNameMapper[SCE_USER_STYLE_COMMENTLINE].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_NUMBER,               globalMappper().styleNameMapper[SCE_USER_STYLE_NUMBER].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD1,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD1].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD2,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD2].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD3,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD3].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD4,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD4].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD5,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD5].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD6,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD6].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD7,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD7].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_KEYWORD8,             globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD8].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_OPERATOR,             globalMappper().styleNameMapper[SCE_USER_STYLE_OPERATOR].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_FOLDER_IN_CODE1,      globalMappper().styleNameMapper[SCE_USER_STYLE_FOLDER_IN_CODE1].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_FOLDER_IN_CODE2,      globalMappper().styleNameMapper[SCE_USER_STYLE_FOLDER_IN_CODE2].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_FOLDER_IN_COMMENT,    globalMappper().styleNameMapper[SCE_USER_STYLE_FOLDER_IN_COMMENT].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER1,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER1].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER2,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER2].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER3,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER3].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER4,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER4].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER5,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER5].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER6,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER6].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER7,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER7].c_str());
    _pCurrentUserLang->_styleArray.addStyler(SCE_USER_STYLE_DELIMITER8,           globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER8].c_str());
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
    for (int i = 0, nb = pNppParam->getNbUserLang(); i < nb ; ++i)
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

INT_PTR CALLBACK UserDefineDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    NppParameters *pNppParam = NppParameters::getInstance();
    switch (message)
    {
        case WM_INITDIALOG :
        {
            _pUserLang = _pCurrentUserLang;

            _ctrlTab.init(_hInst, _hSelf, false);
			int tabDpiDynamicalHeight = NppParameters::getInstance()->_dpiManager.scaleY(13);
            _ctrlTab.setFont(TEXT("Tahoma"), tabDpiDynamicalHeight);

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

            _wVector.push_back(DlgInfo(&_folderStyleDlg,    TEXT("Folder && Default")));
            _wVector.push_back(DlgInfo(&_keyWordsStyleDlg,  TEXT("Keywords Lists")));
            _wVector.push_back(DlgInfo(&_commentStyleDlg,   TEXT("Comment && Number")));
            _wVector.push_back(DlgInfo(&_symbolsStyleDlg,   TEXT("Operators && Delimiters")));

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
            rc.bottom -= 20;
            _ctrlTab.reSizeTo(rc);

            _folderStyleDlg.reSizeTo(rc);
            _keyWordsStyleDlg.reSizeTo(rc);
            _commentStyleDlg.reSizeTo(rc);
            _symbolsStyleDlg.reSizeTo(rc);

            reloadLangCombo();
            ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, 0, 0);

            enableLangAndControlsBy(0);

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
            ::SetScrollInfo(_hSelf, SB_VERT, &si, TRUE);

            TCHAR temp[32];
            generic_string udlVersion = TEXT("User Defined Language v.");
            udlVersion += generic_itoa(SCE_UDL_VERSION_MAJOR, temp, 10);
            udlVersion += TEXT(".");
            udlVersion += generic_itoa(SCE_UDL_VERSION_MINOR, temp, 10);
            udlVersion += TEXT(".");
            udlVersion += generic_itoa(SCE_UDL_VERSION_BUILD, temp, 10);
            udlVersion += TEXT(".");
            udlVersion += generic_itoa(SCE_UDL_VERSION_REVISION, temp, 10);

            ::SetWindowText(_hSelf, udlVersion.c_str());

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

                        int i2Export = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);

                        if (i2Export == 0)
                        {
                            // maybe a better option would be to simply send IDC_SAVEAS_BUTTON message, and display "Save As..." dialog?
                            printStr(TEXT("Before exporting, save your language definition by clicking \"Save As...\" button"));
                            break;
                        }

                        FileDialog fDlg(_hSelf, ::GetModuleHandle(NULL));
                        fDlg.setExtFilter(TEXT("UDL"), TEXT(".xml"), NULL);
                        TCHAR *fn = fDlg.doSaveDlg();
                        if (!fn) break;
                        generic_string fileName2save = fn;

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

INT_PTR CALLBACK StringDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
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

INT_PTR CALLBACK StylerDlg::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    StylerDlg * dlg = (StylerDlg *)::GetProp(hwnd, TEXT("Styler dialog prop"));
    NppParameters *pNppParam = NppParameters::getInstance();

    switch (message)
    {
        case WM_INITDIALOG :
        {
            NativeLangSpeaker *pNativeLangSpeaker = pNppParam->getNativeLangSpeaker();
            pNativeLangSpeaker->changeUserDefineLangPopupDlg(hwnd);

            ::SetProp(hwnd, TEXT("Styler dialog prop"), (HANDLE)lParam);
            dlg = (StylerDlg *)::GetProp(hwnd, TEXT("Styler dialog prop"));
            Style & style = SharedParametersDialog::_pUserLang->_styleArray.getStyler(dlg->_stylerIndex);

            // move dialog over UDL GUI (position 0,0 of UDL window) so it wouldn't cover the code
            RECT wrc;
            ::GetWindowRect(dlg->_parent, &wrc);
            wrc.left = wrc.left < 0 ? 200 : wrc.left;   // if outside of visible area
            wrc.top = wrc.top < 0 ? 200 : wrc.top;
            ::SetWindowPos(hwnd, HWND_TOP, wrc.left, wrc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            ::SendDlgItemMessage(hwnd, IDC_STYLER_CHECK_UNDERLINE, BM_SETCHECK, style._fontStyle & FONTSTYLE_UNDERLINE, 0);
            ::SendDlgItemMessage(hwnd, IDC_STYLER_CHECK_ITALIC,    BM_SETCHECK, style._fontStyle & FONTSTYLE_ITALIC, 0);
            ::SendDlgItemMessage(hwnd, IDC_STYLER_CHECK_BOLD,      BM_SETCHECK, style._fontStyle & FONTSTYLE_BOLD, 0);

            // for the font size combo
            HWND hFontSizeCombo = ::GetDlgItem(hwnd, IDC_STYLER_COMBO_FONT_SIZE);
            for(int j = 0 ; j < int(sizeof(fontSizeStrs))/(3*sizeof(TCHAR)) ; ++j)
                ::SendMessage(hFontSizeCombo, CB_ADDSTRING, 0, (LPARAM)fontSizeStrs[j]);

            TCHAR size[10];
            if (style._fontSize == -1)
                size[0] = '\0';
            else
                wsprintf(size, TEXT("%d"),style._fontSize);

            int i = ::SendMessage(hFontSizeCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)size);
            if (i != CB_ERR)
                ::SendMessage(hFontSizeCombo, CB_SETCURSEL, i, 0);

            // for the font name combo
            HWND hFontNameCombo = ::GetDlgItem(hwnd, IDC_STYLER_COMBO_FONT_NAME);
            const std::vector<generic_string> & fontlist = pNppParam->getFontList();
            for (size_t j = 0, len = fontlist.size() ; j < len ; ++j)
            {
                int k = ::SendMessage(hFontNameCombo, CB_ADDSTRING, 0, (LPARAM)fontlist[j].c_str());
                ::SendMessage(hFontNameCombo, CB_SETITEMDATA, k, (LPARAM)fontlist[j].c_str());
            }

            i = ::SendMessage(hFontNameCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)style._fontName);
            if (i == CB_ERR)
                i = 0;
            ::SendMessage(hFontNameCombo, CB_SETCURSEL, i, 0);

            HWND hFgColourStaticText = ::GetDlgItem(hwnd, IDC_STYLER_FG_STATIC);
            HWND hBgColourStaticText = ::GetDlgItem(hwnd, IDC_STYLER_BG_STATIC);

            if (style._fgColor == COLORREF(-1))
                style._fgColor = black;

            if (style._bgColor == COLORREF(-1))
                style._bgColor = white;

            dlg->_pFgColour->init(dlg->_hInst, hwnd);
            dlg->_pFgColour->setColour(style._fgColor);
            dlg->_pBgColour->init(dlg->_hInst, hwnd);
            dlg->_pBgColour->setColour(style._bgColor);

            POINT p1, p2;
            RECT rc1, rc2;

            ::GetWindowRect(hFgColourStaticText, &rc1);
            ::GetWindowRect(hBgColourStaticText, &rc2);

            p1.x = rc1.left;    p1.y = rc1.top;
            p2.x = rc2.left;    p2.y = rc2.top;

            p1.x += rc1.right - rc1.left;
            p2.x += rc2.right - rc2.left;

            ::ScreenToClient(hwnd, &p1);
            ::ScreenToClient(hwnd, &p2);

            p1.x += 10; p2.x += 10;
            p1.y -= 6; p2.y -= 6;

            ::MoveWindow(dlg->_pFgColour->getHSelf(), p1.x, p1.y, 30, 30, TRUE);
            ::MoveWindow(dlg->_pBgColour->getHSelf(), p2.x, p2.y, 30, 30, TRUE);

            dlg->_pFgColour->display();
            dlg->_pBgColour->display();

            unordered_map<int, int>::iterator iter = globalMappper().nestingMapper.begin();
            for (; iter != globalMappper().nestingMapper.end(); ++iter)
            {
                ::SendDlgItemMessage(hwnd, iter->first, BM_SETCHECK, style._nesting & iter->second, 0);
                ::EnableWindow(::GetDlgItem(hwnd, iter->first), dlg->_enabledNesters & iter->second);
            }
            return TRUE;
        }

        case WM_COMMAND :
        {
            Style & style = SharedParametersDialog::_pUserLang->_styleArray.getStyler(dlg->_stylerIndex);
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int i = ::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
                if (LOWORD(wParam) == IDC_STYLER_COMBO_FONT_SIZE)
                {
                    TCHAR intStr[5];
                    if (i != 0)
                    {
                        ::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETLBTEXT, i, (LPARAM)intStr);
                        if ((!intStr) || (!intStr[0]))
                            style._fontSize = -1;
                        else
                        {
                            TCHAR *finStr;
                            style._fontSize = generic_strtol(intStr, &finStr, 10);
                            if (*finStr != '\0')
                                style._fontSize = -1;
                        }
                    }
                    else
                    {
                        style._fontSize = -1;
                    }
                }
                else if (LOWORD(wParam) == IDC_STYLER_COMBO_FONT_NAME)
                {
                    style._fontName = (TCHAR *)::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETITEMDATA, i, 0);
                }

                // show changes to user, re-color document
                if (SharedParametersDialog::_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
                    SharedParametersDialog::_pScintilla->styleChange();
                return TRUE;
            }
            else if (HIWORD(wParam) == CPN_COLOURPICKED)    // #define CPN_COLOURPICKED (BN_CLICKED)
            {
                if (wParam == IDCANCEL)
                {
                    style = dlg->_initialStyle;

                    // show changes to user, re-color document
                    if (SharedParametersDialog::_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
                        SharedParametersDialog::_pScintilla->styleChange();

                    ::RemoveProp(hwnd, TEXT("Styler dialog prop"));
                    ::EndDialog(hwnd, IDCANCEL);
                    return TRUE;
                }

                if (wParam == IDOK)
                {
                    ::RemoveProp(hwnd, TEXT("Styler dialog prop"));
                    ::EndDialog(hwnd, IDOK);
                    return TRUE;
                }

                style._fgColor = dlg->_pFgColour->getColour();
                style._bgColor = dlg->_pBgColour->getColour();

                if (dlg->_pFgColour->isEnabled())
                    style._colorStyle |= COLORSTYLE_FOREGROUND;
                else
                    style._colorStyle &= ~COLORSTYLE_FOREGROUND;
                if (dlg->_pBgColour->isEnabled())
                    style._colorStyle |= COLORSTYLE_BACKGROUND;
                else
                    style._colorStyle &= ~COLORSTYLE_BACKGROUND;

                style._fontStyle = FONTSTYLE_NONE;
                if (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, IDC_STYLER_CHECK_BOLD), BM_GETCHECK, 0, 0))
                    style._fontStyle |= FONTSTYLE_BOLD;
                if (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, IDC_STYLER_CHECK_ITALIC), BM_GETCHECK, 0, 0))
                    style._fontStyle |= FONTSTYLE_ITALIC;
                if (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, IDC_STYLER_CHECK_UNDERLINE), BM_GETCHECK, 0, 0))
                    style._fontStyle |= FONTSTYLE_UNDERLINE;

                style._nesting = SCE_USER_MASK_NESTING_NONE;
                unordered_map<int, int>::iterator iter = globalMappper().nestingMapper.begin();
                for (; iter != globalMappper().nestingMapper.end(); ++iter)
                {
                    if (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, iter->first), BM_GETCHECK, 0, 0))
                        style._nesting |= iter->second;
                }

                // show changes to user, re-color document
                if (SharedParametersDialog::_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
                    SharedParametersDialog::_pScintilla->styleChange();

                return TRUE;
            }
            return FALSE;
        }
        case WM_CLOSE:
        {
            return TRUE;
        }
        default :
            return FALSE;
    }
}
