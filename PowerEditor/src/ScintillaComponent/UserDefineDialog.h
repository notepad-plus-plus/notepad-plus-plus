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

#pragma once

#include "UserDefineResource.h"
#include "ControlsTab.h"
#include "ColourPicker.h"
#include "Parameters.h"
#include "URLCtrl.h"
#include "tchar.h"
#include "SciLexer.h"
#include <unordered_map>

class ScintillaEditView;
class UserLangContainer;
struct Style;
#define WL_LEN_MAX 1024
#define BOLD_MASK     1
#define ITALIC_MASK   2
const bool DOCK = true;
const bool UNDOCK = false;

class GlobalMappers
{
    public:

		std::unordered_map<generic_string, int> keywordIdMapper;
		std::unordered_map<int, generic_string> keywordNameMapper;

		std::unordered_map<generic_string, int> styleIdMapper;
		std::unordered_map<int, generic_string> styleNameMapper;

		std::unordered_map<generic_string, int> temp;
		std::unordered_map<generic_string, int>::iterator iter;

		std::unordered_map<int, int> nestingMapper;
		std::unordered_map<int, int> dialogMapper;
		std::unordered_map<int, std::string> setLexerMapper;

        // only default constructor is needed
        GlobalMappers()
        {
            // pre 2.0
            temp[TEXT("Operators")]                     = SCE_USER_KWLIST_OPERATORS1;
            temp[TEXT("Folder+")]                       = SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN;
            temp[TEXT("Folder-")]                       = SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE;
            temp[TEXT("Words1")]                        = SCE_USER_KWLIST_KEYWORDS1;
            temp[TEXT("Words2")]                        = SCE_USER_KWLIST_KEYWORDS2;
            temp[TEXT("Words3")]                        = SCE_USER_KWLIST_KEYWORDS3;
            temp[TEXT("Words4")]                        = SCE_USER_KWLIST_KEYWORDS4;

            // in case of duplicate entries, newer string should overwrite old one !
            for (iter = temp.begin(); iter != temp.end(); ++iter)
                keywordNameMapper[iter->second] = iter->first;
            keywordIdMapper.insert(temp.begin(), temp.end());
            temp.clear();

            // 2.0
            temp[TEXT("Comments")]                      = SCE_USER_KWLIST_COMMENTS;
            temp[TEXT("Numbers, additional")]           = SCE_USER_KWLIST_NUMBER_RANGE;
            temp[TEXT("Numbers, prefixes")]             = SCE_USER_KWLIST_NUMBER_PREFIX2;
            temp[TEXT("Numbers, extras with prefixes")] = SCE_USER_KWLIST_NUMBER_EXTRAS2;
            temp[TEXT("Numbers, suffixes")]             = SCE_USER_KWLIST_NUMBER_SUFFIX2;
            temp[TEXT("Operators1")]                    = SCE_USER_KWLIST_OPERATORS1;
            temp[TEXT("Operators2")]                    = SCE_USER_KWLIST_OPERATORS2;
            temp[TEXT("Folders in code1, open")]        = SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN;
            temp[TEXT("Folders in code1, middle")]      = SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE;
            temp[TEXT("Folders in code1, close")]       = SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE;
            temp[TEXT("Folders in code2, open")]        = SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN;
            temp[TEXT("Folders in code2, middle")]      = SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE;
            temp[TEXT("Folders in code2, close")]       = SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE;
            temp[TEXT("Folders in comment, open")]      = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN;
            temp[TEXT("Folders in comment, middle")]    = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE;
            temp[TEXT("Folders in comment, close")]     = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE;
            temp[TEXT("Keywords1")]                     = SCE_USER_KWLIST_KEYWORDS1;
            temp[TEXT("Keywords2")]                     = SCE_USER_KWLIST_KEYWORDS2;
            temp[TEXT("Keywords3")]                     = SCE_USER_KWLIST_KEYWORDS3;
            temp[TEXT("Keywords4")]                     = SCE_USER_KWLIST_KEYWORDS4;
            temp[TEXT("Keywords5")]                     = SCE_USER_KWLIST_KEYWORDS5;
            temp[TEXT("Keywords6")]                     = SCE_USER_KWLIST_KEYWORDS6;
            temp[TEXT("Keywords7")]                     = SCE_USER_KWLIST_KEYWORDS7;
            temp[TEXT("Keywords8")]                     = SCE_USER_KWLIST_KEYWORDS8;
            temp[TEXT("Delimiters")]                    = SCE_USER_KWLIST_DELIMITERS;

            // in case of duplicate entries, newer string should overwrite old one !
            for (iter = temp.begin(); iter != temp.end(); ++iter)
                keywordNameMapper[iter->second] = iter->first;
            keywordIdMapper.insert(temp.begin(), temp.end());
            temp.clear();

            // 2.1
            temp[TEXT("Numbers, prefix1")]              = SCE_USER_KWLIST_NUMBER_PREFIX1;
            temp[TEXT("Numbers, prefix2")]              = SCE_USER_KWLIST_NUMBER_PREFIX2;
            temp[TEXT("Numbers, extras1")]              = SCE_USER_KWLIST_NUMBER_EXTRAS1;
            temp[TEXT("Numbers, extras2")]              = SCE_USER_KWLIST_NUMBER_EXTRAS2;
            temp[TEXT("Numbers, suffix1")]              = SCE_USER_KWLIST_NUMBER_SUFFIX1;
            temp[TEXT("Numbers, suffix2")]              = SCE_USER_KWLIST_NUMBER_SUFFIX2;
            temp[TEXT("Numbers, range")]                = SCE_USER_KWLIST_NUMBER_RANGE;

            // in case of duplicate entries, newer string should overwrite old one !
            for (iter = temp.begin(); iter != temp.end(); ++iter)
                keywordNameMapper[iter->second] = iter->first;
            keywordIdMapper.insert(temp.begin(), temp.end());
            temp.clear();

	        // pre 2.0
	        temp[TEXT("FOLDEROPEN")]           = SCE_USER_STYLE_FOLDER_IN_CODE1;
	        temp[TEXT("FOLDERCLOSE")]          = SCE_USER_STYLE_FOLDER_IN_CODE1;
	        temp[TEXT("KEYWORD1")]             = SCE_USER_STYLE_KEYWORD1;
	        temp[TEXT("KEYWORD2")]             = SCE_USER_STYLE_KEYWORD2;
	        temp[TEXT("KEYWORD3")]             = SCE_USER_STYLE_KEYWORD3;
	        temp[TEXT("KEYWORD4")]             = SCE_USER_STYLE_KEYWORD4;
	        temp[TEXT("COMMENT")]              = SCE_USER_STYLE_COMMENT;
	        temp[TEXT("COMMENT LINE")]         = SCE_USER_STYLE_COMMENTLINE;
	        temp[TEXT("NUMBER")]               = SCE_USER_STYLE_NUMBER;
	        temp[TEXT("OPERATOR")]             = SCE_USER_STYLE_OPERATOR;
	        temp[TEXT("DELIMINER1")]           = SCE_USER_STYLE_DELIMITER1;
	        temp[TEXT("DELIMINER2")]           = SCE_USER_STYLE_DELIMITER2;
	        temp[TEXT("DELIMINER3")]           = SCE_USER_STYLE_DELIMITER3;
	
	        // in case of duplicate entries, newer string should overwrite old one !
	        for (iter = temp.begin(); iter != temp.end(); ++iter)
		        styleNameMapper[iter->second] = iter->first;
	        styleIdMapper.insert(temp.begin(), temp.end());
	        temp.clear();
			
	        // post 2.0
	        temp[TEXT("DEFAULT")]              = SCE_USER_STYLE_DEFAULT;
	        temp[TEXT("COMMENTS")]             = SCE_USER_STYLE_COMMENT;
	        temp[TEXT("LINE COMMENTS")]        = SCE_USER_STYLE_COMMENTLINE;
	        temp[TEXT("NUMBERS")]              = SCE_USER_STYLE_NUMBER;
	        temp[TEXT("KEYWORDS1")]            = SCE_USER_STYLE_KEYWORD1;
	        temp[TEXT("KEYWORDS2")]            = SCE_USER_STYLE_KEYWORD2;
	        temp[TEXT("KEYWORDS3")]            = SCE_USER_STYLE_KEYWORD3;
	        temp[TEXT("KEYWORDS4")]            = SCE_USER_STYLE_KEYWORD4;
	        temp[TEXT("KEYWORDS5")]            = SCE_USER_STYLE_KEYWORD5;
	        temp[TEXT("KEYWORDS6")]            = SCE_USER_STYLE_KEYWORD6;
	        temp[TEXT("KEYWORDS7")]            = SCE_USER_STYLE_KEYWORD7;
	        temp[TEXT("KEYWORDS8")]            = SCE_USER_STYLE_KEYWORD8;
	        temp[TEXT("OPERATORS")]            = SCE_USER_STYLE_OPERATOR;
	        temp[TEXT("FOLDER IN CODE1")]      = SCE_USER_STYLE_FOLDER_IN_CODE1;
	        temp[TEXT("FOLDER IN CODE2")]      = SCE_USER_STYLE_FOLDER_IN_CODE2;
	        temp[TEXT("FOLDER IN COMMENT")]    = SCE_USER_STYLE_FOLDER_IN_COMMENT;
	        temp[TEXT("DELIMITERS1")]          = SCE_USER_STYLE_DELIMITER1;
	        temp[TEXT("DELIMITERS2")]          = SCE_USER_STYLE_DELIMITER2;
	        temp[TEXT("DELIMITERS3")]          = SCE_USER_STYLE_DELIMITER3;
	        temp[TEXT("DELIMITERS4")]          = SCE_USER_STYLE_DELIMITER4;
	        temp[TEXT("DELIMITERS5")]          = SCE_USER_STYLE_DELIMITER5;
	        temp[TEXT("DELIMITERS6")]          = SCE_USER_STYLE_DELIMITER6;
	        temp[TEXT("DELIMITERS7")]          = SCE_USER_STYLE_DELIMITER7;
	        temp[TEXT("DELIMITERS8")]          = SCE_USER_STYLE_DELIMITER8;
	
	        // in case of duplicate entries, newer string should overwrite old one !
	        for (iter = temp.begin(); iter != temp.end(); ++iter)
		        styleNameMapper[iter->second] = iter->first;
	        styleIdMapper.insert(temp.begin(), temp.end());
	        temp.clear();

            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER1]      = SCE_USER_MASK_NESTING_DELIMITER1;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER2]      = SCE_USER_MASK_NESTING_DELIMITER2;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER3]      = SCE_USER_MASK_NESTING_DELIMITER3;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER4]      = SCE_USER_MASK_NESTING_DELIMITER4;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER5]      = SCE_USER_MASK_NESTING_DELIMITER5;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER6]      = SCE_USER_MASK_NESTING_DELIMITER6;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER7]      = SCE_USER_MASK_NESTING_DELIMITER7;
            nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER8]      = SCE_USER_MASK_NESTING_DELIMITER8;
            nestingMapper[IDC_STYLER_CHECK_NESTING_COMMENT]         = SCE_USER_MASK_NESTING_COMMENT;
            nestingMapper[IDC_STYLER_CHECK_NESTING_COMMENT_LINE]    = SCE_USER_MASK_NESTING_COMMENT_LINE;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD1]        = SCE_USER_MASK_NESTING_KEYWORD1;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD2]        = SCE_USER_MASK_NESTING_KEYWORD2;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD3]        = SCE_USER_MASK_NESTING_KEYWORD3;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD4]        = SCE_USER_MASK_NESTING_KEYWORD4;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD5]        = SCE_USER_MASK_NESTING_KEYWORD5;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD6]        = SCE_USER_MASK_NESTING_KEYWORD6;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD7]        = SCE_USER_MASK_NESTING_KEYWORD7;
            nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD8]        = SCE_USER_MASK_NESTING_KEYWORD8;
            nestingMapper[IDC_STYLER_CHECK_NESTING_OPERATORS1]      = SCE_USER_MASK_NESTING_OPERATORS1;
            nestingMapper[IDC_STYLER_CHECK_NESTING_OPERATORS2]      = SCE_USER_MASK_NESTING_OPERATORS2;
            nestingMapper[IDC_STYLER_CHECK_NESTING_NUMBERS]         = SCE_USER_MASK_NESTING_NUMBERS;

            dialogMapper[IDC_NUMBER_PREFIX1_EDIT]           = SCE_USER_KWLIST_NUMBER_PREFIX1;
            dialogMapper[IDC_NUMBER_PREFIX2_EDIT]           = SCE_USER_KWLIST_NUMBER_PREFIX2;
            dialogMapper[IDC_NUMBER_EXTRAS1_EDIT]           = SCE_USER_KWLIST_NUMBER_EXTRAS1;
            dialogMapper[IDC_NUMBER_EXTRAS2_EDIT]           = SCE_USER_KWLIST_NUMBER_EXTRAS2;
            dialogMapper[IDC_NUMBER_SUFFIX1_EDIT]           = SCE_USER_KWLIST_NUMBER_SUFFIX1;
            dialogMapper[IDC_NUMBER_SUFFIX2_EDIT]           = SCE_USER_KWLIST_NUMBER_SUFFIX2;
            dialogMapper[IDC_NUMBER_RANGE_EDIT]             = SCE_USER_KWLIST_NUMBER_RANGE;

            dialogMapper[IDC_FOLDER_IN_CODE1_OPEN_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN;    
            dialogMapper[IDC_FOLDER_IN_CODE1_MIDDLE_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE;  
            dialogMapper[IDC_FOLDER_IN_CODE1_CLOSE_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE;   
            dialogMapper[IDC_FOLDER_IN_CODE2_OPEN_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN;    
            dialogMapper[IDC_FOLDER_IN_CODE2_MIDDLE_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE;  
            dialogMapper[IDC_FOLDER_IN_CODE2_CLOSE_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE;   
            dialogMapper[IDC_FOLDER_IN_COMMENT_OPEN_EDIT]  	= SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN;  
            dialogMapper[IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT] = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE;
            dialogMapper[IDC_FOLDER_IN_COMMENT_CLOSE_EDIT]  = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE;

            dialogMapper[IDC_KEYWORD1_EDIT]                 = SCE_USER_KWLIST_KEYWORDS1;
            dialogMapper[IDC_KEYWORD2_EDIT]                 = SCE_USER_KWLIST_KEYWORDS2;
            dialogMapper[IDC_KEYWORD3_EDIT]                 = SCE_USER_KWLIST_KEYWORDS3;
            dialogMapper[IDC_KEYWORD4_EDIT]                 = SCE_USER_KWLIST_KEYWORDS4;
            dialogMapper[IDC_KEYWORD5_EDIT]                 = SCE_USER_KWLIST_KEYWORDS5;
            dialogMapper[IDC_KEYWORD6_EDIT]                 = SCE_USER_KWLIST_KEYWORDS6;
            dialogMapper[IDC_KEYWORD7_EDIT]                 = SCE_USER_KWLIST_KEYWORDS7;
            dialogMapper[IDC_KEYWORD8_EDIT]                 = SCE_USER_KWLIST_KEYWORDS8;

            setLexerMapper[SCE_USER_KWLIST_COMMENTS] 				= "userDefine.comments";
            setLexerMapper[SCE_USER_KWLIST_DELIMITERS] 				= "userDefine.delimiters";
            setLexerMapper[SCE_USER_KWLIST_OPERATORS1] 				= "userDefine.operators1";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_PREFIX1] 			= "userDefine.numberPrefix1";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_PREFIX2] 			= "userDefine.numberPrefix2";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_EXTRAS1] 			= "userDefine.numberExtras1";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_EXTRAS2] 			= "userDefine.numberExtras2";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_SUFFIX1] 			= "userDefine.numberSuffix1";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_SUFFIX2] 			= "userDefine.numberSuffix2";
            setLexerMapper[SCE_USER_KWLIST_NUMBER_RANGE] 			= "userDefine.numberRange";
            setLexerMapper[SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN] 	= "userDefine.foldersInCode1Open";
            setLexerMapper[SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE] = "userDefine.foldersInCode1Middle";
            setLexerMapper[SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE] 	= "userDefine.foldersInCode1Close";
        };
};

GlobalMappers & globalMappper();

class SharedParametersDialog : public StaticDialog
{
friend class StylerDlg;
public:
    SharedParametersDialog() = default;
    virtual void updateDlg() = 0;
protected :
    //Shared data
    static UserLangContainer *_pUserLang;
    static ScintillaEditView *_pScintilla;
    intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    bool setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set);
    virtual void setKeywords2List(int ctrlID) = 0;
};

class FolderStyleDialog : public SharedParametersDialog
{
public:
    FolderStyleDialog() = default;
    void updateDlg();
protected :
    intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int ctrlID);
private :
    void retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR *prefix) const;
    URLCtrl _pageLink;
};

class KeyWordsStyleDialog : public SharedParametersDialog
{
public:
    KeyWordsStyleDialog() = default;
    void updateDlg();
protected :
    intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int id);
};

class CommentStyleDialog : public SharedParametersDialog
{
public :
    CommentStyleDialog() = default;
    void updateDlg();
protected :
    intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int id);
private :
    void retrieve(TCHAR *dest, const TCHAR *toRetrieve, const TCHAR *prefix) const;
};

class SymbolsStyleDialog : public SharedParametersDialog
{
public :
    SymbolsStyleDialog() = default;
    void updateDlg();
protected :
    intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int id);
private :
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
     virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true) {
        StaticDialog::create(dialogID, isRTL, msgDestParent);
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
    virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private :
    ControlsTab _ctrlTab;
    WindowVector _wVector;
    UserLangContainer *_pCurrentUserLang = nullptr;
    FolderStyleDialog       _folderStyleDlg;
    KeyWordsStyleDialog     _keyWordsStyleDlg;
    CommentStyleDialog      _commentStyleDlg;
    SymbolsStyleDialog      _symbolsStyleDlg;
    bool _status = UNDOCK;
    RECT _dlgPos = {};
    int _currentHight = 0;
    int _yScrollPos = 0;
    int _prevHightVal = 0;
    void getActualPosSize() {
        ::GetWindowRect(_hSelf, &_dlgPos);
        _dlgPos.right -= _dlgPos.left;
        _dlgPos.bottom -= _dlgPos.top;
    };
    void restorePosSize(){reSizeTo(_dlgPos);};
    void enableLangAndControlsBy(size_t index);
protected :
    void setKeywords2List(int){};
    void updateDlg();
};

class StringDlg : public StaticDialog
{
public :
    StringDlg() = default;
	void init(HINSTANCE hInst, HWND parent, const TCHAR *title, const TCHAR *staticName, const TCHAR *text2Set, int txtLen = 0, const TCHAR* restrictedChars = nullptr, bool bGotoCenter = false) {
		Window::init(hInst, parent);
		_title = title;
		_static = staticName;
		_textValue = text2Set;
		_txtLen = txtLen;
		_shouldGotoCenter = bGotoCenter;
		if (restrictedChars && _tcslen(restrictedChars))
		{
			_restrictedChars = restrictedChars;
		}
	};

    intptr_t doDialog() {
        return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_STRING_DLG), _hParent,  dlgProc, reinterpret_cast<LPARAM>(this));
    };

    virtual void destroy() {};
	
protected :
    intptr_t CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM);

	// Custom proc to subclass edit control
	LRESULT static CALLBACK customEditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam);

	bool isAllowed(const generic_string& txt);
	void HandlePaste(HWND hEdit);

private :
    generic_string _title;
    generic_string _textValue;
    generic_string _static;
	generic_string _restrictedChars;
    int _txtLen = 0;
	bool _shouldGotoCenter = false;
	WNDPROC _oldEditProc = nullptr;
};

class StylerDlg
{
public:
    StylerDlg( HINSTANCE hInst, HWND parent, int stylerIndex = 0, int enabledNesters = -1):
        _hInst(hInst), _parent(parent), _stylerIndex(stylerIndex), _enabledNesters(enabledNesters) {
        _pFgColour = new ColourPicker;
        _pBgColour = new ColourPicker;
        _initialStyle = SharedParametersDialog::_pUserLang->_styles.getStyler(stylerIndex);
    };

    ~StylerDlg() {
        _pFgColour->destroy();
        _pBgColour->destroy();
        delete _pFgColour;
        delete _pBgColour;
	};

    long doDialog() {
		return long(::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_STYLER_POPUP_DLG), _parent, dlgProc, reinterpret_cast<LPARAM>(this)));
    };

    static intptr_t CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE _hInst = nullptr;
    HWND _parent = nullptr;
    int _stylerIndex = 0;
    int _enabledNesters = 0;
    ColourPicker * _pFgColour = nullptr;
    ColourPicker * _pBgColour = nullptr;
    Style _initialStyle;

    void move2CtrlRight(HWND hwndDlg, int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight);
};
