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
#ifndef USER_DEFINE_H
#define USER_DEFINE_H
#ifndef USERDEFINE_RC_H
#include "UserDefineResource.h"
#endif //USERDEFINE_RC_H
#ifndef CONTROLS_TAB_H
#include "ControlsTab.h"
#endif //CONTROLS_TAB_H
#ifndef COLOUR_PICKER_H
#include "ColourPicker.h"
#endif //COLOUR_PICKER_H
#ifndef PARAMETERS_H
#include "Parameters.h"
#endif //PARAMETERS_H
#ifndef URLCTRL_INCLUDED
#include "URLCtrl.h"
#endif// URLCTRL_INCLUDED
#ifdef __GNUC__
static int min(int a, int b) {
    return (a<b)?a:b;
};
static int max(int a, int b) {
    return (a>b)?a:b;
};
#endif //__GNUC__
#include "tchar.h"
#include "scilexer.h"
#include <map>
using namespace std;
class ScintillaEditView;
class UserLangContainer;
struct Style;
#define WL_LEN_MAX 1024
#define BOLD_MASK     1
#define ITALIC_MASK   2
const bool DOCK = true;
const bool UNDOCK = false;
const TCHAR keywordListMapper[SCE_USER_KWLIST_TOTAL][32] = {
    TEXT("Comments"),                       // SCE_USER_KWLIST_COMMENTS
    TEXT("Numbers, additional"),            // SCE_USER_KWLIST_NUMBER_EXTRA
    TEXT("Numbers, prefixes"),              // SCE_USER_KWLIST_NUMBER_PREFIX
    TEXT("Numbers, extras with prefixes"),  // SCE_USER_KWLIST_NUMBER_EXTRAPREF
    TEXT("Numbers, suffixes"),              // SCE_USER_KWLIST_NUMBER_SUFFIX
    TEXT("Operators1"),                     // SCE_USER_KWLIST_OPERATORS1
    TEXT("Operators2"),                     // SCE_USER_KWLIST_OPERATORS2
    TEXT("Folders in code1, open"),         // SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN
    TEXT("Folders in code1, middle"),       // SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE
    TEXT("Folders in code1, close"),        // SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE
    TEXT("Folders in code2, open"),         // SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN
    TEXT("Folders in code2, middle"),       // SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE
    TEXT("Folders in code2, close"),        // SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE
    TEXT("Folders in comment, open"),       // SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN
    TEXT("Folders in comment, middle"),     // SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE
    TEXT("Folders in comment, close"),      // SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE
    TEXT("Keywords1"),                      // SCE_USER_KWLIST_KEYWORDS1
    TEXT("Keywords2"),                      // SCE_USER_KWLIST_KEYWORDS2
    TEXT("Keywords3"),                      // SCE_USER_KWLIST_KEYWORDS3
    TEXT("Keywords4"),                      // SCE_USER_KWLIST_KEYWORDS4
    TEXT("Keywords5"),                      // SCE_USER_KWLIST_KEYWORDS5
    TEXT("Keywords6"),                      // SCE_USER_KWLIST_KEYWORDS6
    TEXT("Keywords7"),                      // SCE_USER_KWLIST_KEYWORDS7
    TEXT("Keywords8"),                      // SCE_USER_KWLIST_KEYWORDS8
    TEXT("Delimiters")                      // SCE_USER_KWLIST_DELIMITERS
};
const TCHAR styleNameMapper[SCE_USER_STYLE_TOTAL_STYLES][32] = {
    TEXT("DEFAULT"),                    // 0   SCE_USER_STYLE_DEFAULT
    TEXT("COMMENTS"),                   // 1   SCE_USER_STYLE_COMMENT
    TEXT("LINE COMMENTS"),              // 2   SCE_USER_STYLE_COMMENTLINE
    TEXT("NUMBERS"),                    // 3   SCE_USER_STYLE_NUMBER
    TEXT("KEYWORDS1"),                  // 4   SCE_USER_STYLE_KEYWORD1
    TEXT("KEYWORDS2"),                  // 5   SCE_USER_STYLE_KEYWORD2
    TEXT("KEYWORDS3"),                  // 6   SCE_USER_STYLE_KEYWORD3
    TEXT("KEYWORDS4"),                  // 7   SCE_USER_STYLE_KEYWORD4
    TEXT("KEYWORDS5"),                  // 8   SCE_USER_STYLE_KEYWORD5
    TEXT("KEYWORDS6"),                  // 9   SCE_USER_STYLE_KEYWORD6
    TEXT("KEYWORDS7"),                  // 10  SCE_USER_STYLE_KEYWORD7
    TEXT("KEYWORDS8"),                  // 11  SCE_USER_STYLE_KEYWORD8
    TEXT("OPERATORS"),                  // 12  SCE_USER_STYLE_OPERATOR
    TEXT("FOLDER IN CODE1"),            // 13  SCE_USER_STYLE_FOLDER_IN_CODE1
    TEXT("FOLDER IN CODE2"),            // 14  SCE_USER_STYLE_FOLDER_IN_CODE2
    TEXT("FOLDER IN COMMENT"),          // 15  SCE_USER_STYLE_FOLDER_IN_COMMENT
    TEXT("DELIMITERS1"),                // 16  SCE_USER_STYLE_DELIMITER1
    TEXT("DELIMITERS2"),                // 17  SCE_USER_STYLE_DELIMITER2
    TEXT("DELIMITERS3"),                // 18  SCE_USER_STYLE_DELIMITER3
    TEXT("DELIMITERS4"),                // 19  SCE_USER_STYLE_DELIMITER4
    TEXT("DELIMITERS5"),                // 20  SCE_USER_STYLE_DELIMITER5
    TEXT("DELIMITERS6"),                // 21  SCE_USER_STYLE_DELIMITER6
    TEXT("DELIMITERS7"),                // 22  SCE_USER_STYLE_DELIMITER7
    TEXT("DELIMITERS8")                 // 23  SCE_USER_STYLE_DELIMITER8
};
const int styleIdMApper[SCE_USER_STYLE_MAPPER_TOTAL] = {            // mapping sytle ID's between UDL 1.0 and UDL 2.0
    -1, // -1 stands for: not used      // 0
    SCE_USER_STYLE_COMMENT,             // 1
    SCE_USER_STYLE_COMMENTLINE,         // 2
    -1,                                 // 3
    SCE_USER_STYLE_NUMBER,              // 4
    SCE_USER_STYLE_KEYWORD1,            // 5
    SCE_USER_STYLE_KEYWORD2,            // 6
    SCE_USER_STYLE_KEYWORD3,            // 7
    SCE_USER_STYLE_KEYWORD4,            // 8
    -1,                                 // 9
    SCE_USER_STYLE_OPERATOR,            // 10
    SCE_USER_STYLE_DEFAULT,             // 11
    SCE_USER_STYLE_FOLDER_IN_CODE1,     // 12
    -1,                                 // 13
    SCE_USER_STYLE_DELIMITER1,          // 14
    SCE_USER_STYLE_DELIMITER2,          // 15
    SCE_USER_STYLE_DELIMITER3,          // 16
};
const int nestingMapper[][2] = {
    { IDC_STYLER_CHECK_NESTING_DELIMITER1,      SCE_USER_MASK_NESTING_DELIMITER1 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER2,      SCE_USER_MASK_NESTING_DELIMITER2 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER3,      SCE_USER_MASK_NESTING_DELIMITER3 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER4,      SCE_USER_MASK_NESTING_DELIMITER4 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER5,      SCE_USER_MASK_NESTING_DELIMITER5 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER6,      SCE_USER_MASK_NESTING_DELIMITER6 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER7,      SCE_USER_MASK_NESTING_DELIMITER7 },
    { IDC_STYLER_CHECK_NESTING_DELIMITER8,      SCE_USER_MASK_NESTING_DELIMITER8 },
    { IDC_STYLER_CHECK_NESTING_COMMENT,         SCE_USER_MASK_NESTING_COMMENT },
    { IDC_STYLER_CHECK_NESTING_COMMENT_LINE,    SCE_USER_MASK_NESTING_COMMENT_LINE },
    { IDC_STYLER_CHECK_NESTING_KEYWORD1,        SCE_USER_MASK_NESTING_KEYWORD1 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD2,        SCE_USER_MASK_NESTING_KEYWORD2 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD3,        SCE_USER_MASK_NESTING_KEYWORD3 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD4,        SCE_USER_MASK_NESTING_KEYWORD4 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD5,        SCE_USER_MASK_NESTING_KEYWORD5 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD6,        SCE_USER_MASK_NESTING_KEYWORD6 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD7,        SCE_USER_MASK_NESTING_KEYWORD7 },
    { IDC_STYLER_CHECK_NESTING_KEYWORD8,        SCE_USER_MASK_NESTING_KEYWORD8 },
    { IDC_STYLER_CHECK_NESTING_OPERATORS1,      SCE_USER_MASK_NESTING_OPERATORS1 },
    { IDC_STYLER_CHECK_NESTING_OPERATORS2,      SCE_USER_MASK_NESTING_OPERATORS2 },
    { IDC_STYLER_CHECK_NESTING_NUMBERS,         SCE_USER_MASK_NESTING_NUMBERS }
};
class SharedParametersDialog : public StaticDialog
{
friend class StylerDlg;
public:
    SharedParametersDialog() {};
    virtual void updateDlg() = 0;
protected :
    //Shared data
    static UserLangContainer *_pUserLang;
    static ScintillaEditView *_pScintilla;
    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    bool setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set);
    virtual void setKeywords2List(int ctrlID) = 0;
};
class FolderStyleDialog : public SharedParametersDialog
{
public:
    FolderStyleDialog(): SharedParametersDialog() {};
    void updateDlg();
protected :
    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int ctrlID);
private :
    void convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR *prefix) const;
    void retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const;
    URLCtrl _pageLink;
};
class KeyWordsStyleDialog : public SharedParametersDialog
{
public:
    KeyWordsStyleDialog(): SharedParametersDialog() {};
    void updateDlg();
protected :
    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int id);
};
class CommentStyleDialog : public SharedParametersDialog
{
public :
    CommentStyleDialog(): SharedParametersDialog() {};
    void updateDlg();
protected :
    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int id);
private :
    void convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR *prefix) const;
    void retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const;
};
class SymbolsStyleDialog : public SharedParametersDialog
{
public :
    SymbolsStyleDialog(): SharedParametersDialog() {};
    void updateDlg();
protected :
    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int id);
private :
    void convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR *prefix) const;
    void retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const;
};
class UserDefineDialog : public SharedParametersDialog
{
friend class ScintillaEditView;
public :
    UserDefineDialog();
    ~UserDefineDialog();
    void init(HINSTANCE hInst, HWND hPere, ScintillaEditView *pSev) {
        if (!_pScintilla)
        {
            Window::init(hInst, hPere);
            _pScintilla = pSev;
        }
    };
    void setScintilla(ScintillaEditView *pScinView) {
        _pScintilla = pScinView;
    };
     virtual void create(int dialogID, bool isRTL = false) {
        StaticDialog::create(dialogID, isRTL);
    }
    void destroy() {
        // A Ajouter les fils...
    };
    int getWidth() const {
        return _dlgPos.right;
    };
    int getHeight() const {
        return _dlgPos.bottom;
    };
    void doDialog(bool willBeShown = true, bool isRTL = false) {
        if (!isCreated())
            create(IDD_GLOBAL_USERDEFINE_DLG, isRTL);
        display(willBeShown);
    };
    virtual void reSizeTo(RECT & rc) // should NEVER be const !!!
    {
        Window::reSizeTo(rc);
        display(false);
        display();
    };
    void reloadLangCombo();
    void changeStyle();
    bool isDocked() const {return _status == DOCK;};
    void setDockStatus(bool isDocked) {_status = isDocked;};
    bool isDirty() const {return _isDirty;};
    HWND getFolderHandle() const {
        return _folderStyleDlg.getHSelf();
    };
    HWND getKeywordsHandle() const {
        return _keyWordsStyleDlg.getHSelf();
    };
    HWND getCommentHandle() const {
        return _commentStyleDlg.getHSelf();
    };
    HWND getSymbolHandle() const {
        return _symbolsStyleDlg.getHSelf();
    };
    void setTabName(int index, const TCHAR *name2set) {
        _ctrlTab.renameTab(index, name2set);
    };
protected :
    virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private :
    ControlsTab _ctrlTab;
    WindowVector _wVector;
    UserLangContainer *_pCurrentUserLang;
    FolderStyleDialog       _folderStyleDlg;
    KeyWordsStyleDialog     _keyWordsStyleDlg;
    CommentStyleDialog      _commentStyleDlg;
    SymbolsStyleDialog      _symbolsStyleDlg;
    bool _status;
    RECT _dlgPos;
    int _currentHight;
    int _yScrollPos;
    int _prevHightVal;
    bool _isDirty;
    void getActualPosSize() {
        ::GetWindowRect(_hSelf, &_dlgPos);
        _dlgPos.right -= _dlgPos.left;
        _dlgPos.bottom -= _dlgPos.top;
    };
    void restorePosSize(){reSizeTo(_dlgPos);};
    void enableLangAndControlsBy(int index);
protected :
    void setKeywords2List(int){};
    void updateDlg();
};
class StringDlg : public StaticDialog
{
public :
    StringDlg() : StaticDialog() {};
    void init(HINSTANCE hInst, HWND parent, TCHAR *title, TCHAR *staticName, TCHAR *text2Set, int txtLen = 0) {
        Window::init(hInst, parent);
        _title = title;
        _static = staticName;
        _textValue = text2Set;
        _txtLen = txtLen;
    };
    long doDialog() {
        return long(::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_STRING_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this));
    };
    virtual void destroy() {};
protected :
    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);
private :
    generic_string _title;
    generic_string _textValue;
    generic_string _static;
    int _txtLen;
};
class StylerDlg
{
public:
    StylerDlg( HINSTANCE hInst, HWND parent, int stylerIndex = 0, int enabledNesters = -1):
        hInst(hInst), parent(parent), stylerIndex(stylerIndex), enabledNesters(enabledNesters)
    {
        pFgColour = new ColourPicker;
        pBgColour = new ColourPicker;
        initialStyle = SharedParametersDialog::_pUserLang->_styleArray.getStyler(stylerIndex);
    };
    ~StylerDlg()
    {
        pFgColour->destroy();
        pBgColour->destroy();
        delete pFgColour;
        delete pBgColour;
    }
    long doDialog() {
        return long (::DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_STYLER_POPUP_DLG), parent,  (DLGPROC)dlgProc, (LPARAM)this));
    };
    static BOOL CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
    HINSTANCE hInst;
    HWND parent;
    int stylerIndex;
    int enabledNesters;
    ColourPicker * pFgColour;
    ColourPicker * pBgColour;
    Style initialStyle;
};
#endif //USER_DEFINE_H
