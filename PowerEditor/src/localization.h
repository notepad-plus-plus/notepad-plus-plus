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

#include "Common.h"
#include "tinyxmlA.h"


class FindReplaceDlg;
class PreferenceDlg;
class ShortcutMapper;
class UserDefineDialog;
class PluginsAdminDlg;

class MenuPosition {
public:
	int _x = -1; // menu
	int _y = -1; // sub-menu
	int _z = -1; // sub-sub-menu
	char _id[64] = { '\0' }; // a unique string defined in localization XML file
};


class NativeLangSpeaker {
public:
    NativeLangSpeaker():_nativeLangA(NULL), _nativeLangEncoding(CP_ACP), _isRTL(false), _fileName(NULL){};
    void init(TiXmlDocumentA *nativeLangDocRootA, bool loadIfEnglish = false);
	void changeConfigLang(HWND hDlg);
	void changeLangTabContextMenu(HMENU hCM);
	TiXmlNodeA * searchDlgNode(TiXmlNodeA *node, const char *dlgTagName);
	bool changeDlgLang(HWND hDlg, const char *dlgTagName, char *title = NULL, size_t titleMaxSize = 0);
	void changeLangTabDrapContextMenu(HMENU hCM);
	generic_string getSpecialMenuEntryName(const char *entryName) const;
	generic_string getNativeLangMenuString(int itemID) const;
	generic_string getShortcutNameString(int itemID) const;

	void changeMenuLang(HMENU menuHandle);
	void changeShortcutLang();
	void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
    void changeUserDefineLang(UserDefineDialog *userDefineDlg);
	void changeUserDefineLangPopupDlg(HWND hDlg);
    void changeFindReplaceDlgLang(FindReplaceDlg & findReplaceDlg);
    void changePrefereceDlgLang(PreferenceDlg & preference);
	void changePluginsAdminDlgLang(PluginsAdminDlg & pluginsAdminDlg);

	bool getDoSaveOrNotStrings(generic_string& title, generic_string& msg);

    bool isRTL() const {
        return _isRTL;
    };

    const char * getFileName() const {
        return _fileName;
    };

    const TiXmlNodeA * getNativeLangA() {
        return _nativeLangA;
    };

    int getLangEncoding() const {
        return _nativeLangEncoding;
    };
	bool getMsgBoxLang(const char *msgBoxTagName, generic_string & title, generic_string & message);
	generic_string getShortcutMapperLangStr(const char *nodeName, const TCHAR *defaultStr) const;
	generic_string getProjectPanelLangMenuStr(const char * nodeName, int cmdID, const TCHAR *defaultStr) const;
	generic_string getFileBrowserLangMenuStr(int cmdID, const TCHAR *defaultStr) const;
	generic_string getAttrNameStr(const TCHAR *defaultStr, const char *nodeL1Name, const char *nodeL2Name, const char *nodeL3Name = "name") const;
	generic_string getLocalizedStrFromID(const char *strID, const generic_string& defaultString) const;

	int messageBox(const char *msgBoxTagName, HWND hWnd, const TCHAR *message, const TCHAR *title, int msgBoxType, int intInfo = 0, const TCHAR *strInfo = NULL);
private:
	TiXmlNodeA *_nativeLangA;
	int _nativeLangEncoding;
    bool _isRTL;
    const char *_fileName;
};


MenuPosition & getMenuPosition(const char *id);

