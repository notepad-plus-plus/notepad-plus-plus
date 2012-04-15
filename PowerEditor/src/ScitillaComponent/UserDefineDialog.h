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

#ifdef __GNUC__
static int min(int a, int b) {
	return (a<b)?a:b;
};

static int max(int a, int b) {
	return (a>b)?a:b;
};
#endif //__GNUC__

class ScintillaEditView;
class UserLangContainer;
struct Style;

#define WL_LEN_MAX 1024

#define BOLD_MASK     1
#define ITALIC_MASK   2

const int nbWordList = 4;
const int nbBlockColor = 5;
const int nbBoolean = 5;

const bool DOCK = true;
const bool UNDOCK = false;

const int maxNbGroup = 10;

const int KWL_FOLDER_OPEN_INDEX = 1;
const int KWL_FOLDER_CLOSE_INDEX = 2;
const int KWL_OPERATOR_INDEX = 3;
const int KWL_COMMENT_INDEX = 4;
const int KWL_KW1_INDEX = 5;
const int KWL_KW2_INDEX = 6;
const int KWL_KW3_INDEX = 7;
const int KWL_KW4_INDEX = 8;
const int KWL_DELIM_INDEX = 0;

const int STYLE_DEFAULT_INDEX = 0;
const int STYLE_BLOCK_OPEN_INDEX = 1;
const int STYLE_BLOCK_CLOSE_INDEX = 2;
const int STYLE_WORD1_INDEX = 3;
const int STYLE_WORD2_INDEX = 4;
const int STYLE_WORD3_INDEX = 5;
const int STYLE_WORD4_INDEX = 6;
const int STYLE_COMMENT_INDEX = 7;
const int STYLE_COMMENTLINE_INDEX = 8;
const int STYLE_NUMBER_INDEX = 9;
const int STYLE_OPERATOR_INDEX = 10;
const int STYLE_DELIM2_INDEX = 11;
const int STYLE_DELIM3_INDEX = 12;



class SharedParametersDialog : public StaticDialog
{
public:
	SharedParametersDialog() {};
	SharedParametersDialog(int nbGroup) : _nbGroup(nbGroup) {};
	virtual void updateDlg() = 0;


protected :
	//Shared data
	static UserLangContainer *_pUserLang;
	static ScintillaEditView *_pScintilla;
	
	//data for per object
	int _nbGroup;
	ColourPicker *_pFgColour[maxNbGroup];
    ColourPicker *_pBgColour[maxNbGroup];
	int _fgStatic[maxNbGroup];
	int _bgStatic[maxNbGroup];
	int _fontSizeCombo[maxNbGroup];
	int _fontNameCombo[maxNbGroup];

    BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void initControls();
	void styleUpdate(const Style & style, ColourPicker *pFgColourPicker, ColourPicker *pBgColourPicker, 
					 int fontComboId, int fontSizeComboId, int boldCheckId, int italicCheckId, int underlineCheckId);

	bool setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set);
	virtual void setKeywords2List(int ctrlID) = 0;
	virtual int getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const = 0;
	virtual int getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const = 0;
	virtual int getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const = 0;
	

};

class FolderStyleDialog : public SharedParametersDialog
{
public:
	FolderStyleDialog();
	void updateDlg();
protected :
	void setKeywords2List(int ctrlID);
	
	int getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const;
	int getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const;
	int getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const;
};

class KeyWordsStyleDialog : public SharedParametersDialog
{
public:
	KeyWordsStyleDialog() ;
	void updateDlg();

protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	void setKeywords2List(int id);

    // SEE @REF #01
    int getGroupIndexFromCombo(int ctrlID, bool & isFontSize)  const;
    int getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const;
    int getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const;
};

class CommentStyleDialog : public SharedParametersDialog
{
public :
    CommentStyleDialog();
    void updateDlg();
protected :
	
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

    void setKeywords2List(int id);
    int getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const;

    int getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const;
    int getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const;

private :
    void convertTo(TCHAR *dest, const TCHAR *toConvert, TCHAR prefix) const;
	void retrieve(TCHAR *dest, const TCHAR *toRetrieve, TCHAR prefix) const;
};

class SymbolsStyleDialog : public SharedParametersDialog 
{
public :
	static const bool ADD;
	static const bool REMOVE;
	SymbolsStyleDialog();
	void updateDlg();
	void undeleteChar();

protected :
	BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
    void setKeywords2List(int) {};
    int getGroupIndexFromCombo(int ctrlID, bool & isFontSize) const;
    int getStylerIndexFromCP(HWND hWnd, bool & isFG, ColourPicker **ppCP) const;
    int getGroupeIndexFromCheck(int ctrlID, int & fontStyleMask) const;

private :
	// 2 static const TCHAR * to have the compatibility with the old xml
	static const TCHAR *_delimTag1;
	static const TCHAR *_delimTag2;
	TCHAR _lastEscapeChar;

	void symbolAction(bool action);
	void listboxsRemoveAll();
	void listboxsInit();
	void listboxsReInit() {
		listboxsRemoveAll();
		listboxsInit();
	};
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

	int getNbKeywordList() {return nbKeywodList;};
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

	FolderStyleDialog		_folderStyleDlg;
	KeyWordsStyleDialog		_keyWordsStyleDlg;
	CommentStyleDialog		_commentStyleDlg;
	SymbolsStyleDialog		_symbolsStyleDlg;    

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
    int getGroupIndexFromCombo(int, bool &) const {return -1;};
    int getStylerIndexFromCP(HWND, bool &, ColourPicker **) const {return -1;};
    int getGroupeIndexFromCheck(int, int &) const {return -1;};
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


#endif //USER_DEFINE_H
