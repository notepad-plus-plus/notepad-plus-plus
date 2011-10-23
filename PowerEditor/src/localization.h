//this file is part of notepad++
//Copyright (C)2010 Don HO <don.h@free.fr>
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

#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#ifndef TINYXMLA_INCLUDED
#include "tinyxmlA.h"
#endif //TINYXMLA_INCLUDED

class FindReplaceDlg;
class PreferenceDlg;
class ShortcutMapper;
class UserDefineDialog;

class NativeLangSpeaker {
public:
    NativeLangSpeaker():_nativeLangA(NULL), _nativeLangEncoding(CP_ACP), _isRTL(false), _fileName(NULL){};
    void init(TiXmlDocumentA *nativeLangDocRootA, bool loadIfEnglish = false);
	void changeConfigLang(HWND hDlg);
	void changeLangTabContextMenu(HMENU hCM);
	TiXmlNodeA * searchDlgNode(TiXmlNodeA *node, const char *dlgTagName);
	bool changeDlgLang(HWND hDlg, const char *dlgTagName, char *title = NULL);
	void changeLangTabDrapContextMenu(HMENU hCM);
	generic_string getSpecialMenuEntryName(const char *entryName);
	generic_string getNativeLangMenuString(int itemID);
	void changeMenuLang(HMENU menuHandle, generic_string & pluginsTrans, generic_string & windowTrans);
	void changeShortcutLang();
	void changeShortcutmapperLang(ShortcutMapper * sm);
	void changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText);
    void changeUserDefineLang(UserDefineDialog *userDefineDlg);
    void changeFindReplaceDlgLang(FindReplaceDlg & findReplaceDlg);
    void changePrefereceDlgLang(PreferenceDlg & preference);
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
	generic_string getProjectPanelLangMenuStr(const char * nodeName, int cmdID, const TCHAR *defaultStr) const;
	generic_string getProjectPanelLangStr(const char *nodeName, const TCHAR *defaultStr) const;
	int messageBox(const char *msgBoxTagName, HWND hWnd, TCHAR *message, TCHAR *title, int msgBoxType);
private:
	TiXmlNodeA *_nativeLangA;
	int _nativeLangEncoding;
    bool _isRTL;
    const char *_fileName;
};

#endif // LOCALIZATION_H
