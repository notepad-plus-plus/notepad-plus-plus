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


#include <algorithm>
#include <shlwapi.h>
#include "Notepad_plus_Window.h"
#include "TaskListDlg.h"
#include "ImageListSet.h"
#include "ShortcutMapper.h"
#include "ansiCharPanel.h"
#include "clipboardHistoryPanel.h"
#include "VerticalFileSwitcher.h"
#include "ProjectPanel.h"
#include "documentMap.h"
#include "functionListPanel.h"
#include "fileBrowser.h"

using namespace std;

#define WM_DPICHANGED 0x02E0


struct SortTaskListPred final
{
	DocTabView *_views[2];

	SortTaskListPred(DocTabView &p, DocTabView &s)
	{
		_views[MAIN_VIEW] = &p;
		_views[SUB_VIEW] = &s;
	}

	bool operator()(const TaskLstFnStatus &l, const TaskLstFnStatus &r) const {
		BufferID lID = _views[l._iView]->getBufferByIndex(l._docIndex);
		BufferID rID = _views[r._iView]->getBufferByIndex(r._docIndex);
		Buffer * bufL = MainFileManager->getBufferByID(lID);
		Buffer * bufR = MainFileManager->getBufferByID(rID);
		return bufL->getRecentTag() > bufR->getRecentTag();
	}
};


LRESULT CALLBACK Notepad_plus_Window::Notepad_plus_Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (hwnd == NULL)
		return FALSE;

	switch(message)
	{
		case WM_NCCREATE:
		{
			// First message we get the ptr of instantiated object
			// then stock it into GWLP_USERDATA index in order to retrieve afterward
			Notepad_plus_Window *pM30ide = static_cast<Notepad_plus_Window *>((reinterpret_cast<LPCREATESTRUCT>(lParam))->lpCreateParams);
			pM30ide->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pM30ide));
			return TRUE;
		}

		default:
		{
			return (reinterpret_cast<Notepad_plus_Window *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA))->runProc(hwnd, message, wParam, lParam));
		}
	}
}


LRESULT Notepad_plus_Window::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			try
			{
				_notepad_plus_plus_core._pPublicInterface = this;
				return _notepad_plus_plus_core.init(hwnd);
			}
			catch (std::exception& ex)
			{
				::MessageBoxA(hwnd, ex.what(), "Exception On WM_CREATE", MB_OK);
				exit(-1);
			}
			break;
		}
		default:
		{
			if (this)
				return _notepad_plus_plus_core.process(hwnd, message, wParam, lParam);
		}
	}
	return FALSE;
}

// Used by NPPM_GETFILENAMEATCURSOR
int CharacterIs(TCHAR c, const TCHAR *any)
{
	int i;
	for (i = 0; any[i] != 0; i++)
	{
		if (any[i] == c) return TRUE;
	}
	return FALSE;
}

LRESULT Notepad_plus::process(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = FALSE;
	NppParameters *pNppParam = NppParameters::getInstance();

	switch (message)
	{
		case WM_NCACTIVATE:
		{
			// Note: lParam is -1 to prevent endless loops of calls
			::SendMessage(_dockingManager.getHSelf(), WM_NCACTIVATE, wParam, -1);
			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
			if (dis->CtlType == ODT_TAB)
				return ::SendMessage(dis->hwndItem, WM_DRAWITEM, wParam, lParam);
			break;
		}

		case WM_DOCK_USERDEFINE_DLG:
		{
			dockUserDlg();
			return TRUE;
		}

		case WM_UNDOCK_USERDEFINE_DLG:
		{
			undockUserDlg();
			return TRUE;
		}

		case WM_REMOVE_USERLANG:
		{
			TCHAR *userLangName = reinterpret_cast<TCHAR *>(lParam);
			if (!userLangName || !userLangName[0])
				return FALSE;

			generic_string name{userLangName};

			//loop through buffers and reset the language (L_USER, TEXT("")) if (L_USER, name)
			for (size_t i = 0; i < MainFileManager->getNbBuffers(); ++i)
			{
				Buffer* buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && name == buf->getUserDefineLangName())
					buf->setLangType(L_USER, TEXT(""));
			}
			return TRUE;
		}

		case WM_RENAME_USERLANG:
		{
			if (!lParam || !((reinterpret_cast<TCHAR *>(lParam))[0]) || !wParam || !((reinterpret_cast<TCHAR *>(wParam))[0]))
				return FALSE;

			generic_string oldName{ reinterpret_cast<TCHAR *>(lParam) };
			generic_string newName{ reinterpret_cast<TCHAR *>(wParam) };

			//loop through buffers and reset the language (L_USER, newName) if (L_USER, oldName)
			for (size_t i = 0; i < MainFileManager->getNbBuffers(); ++i)
			{
				Buffer* buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && oldName == buf->getUserDefineLangName())
					buf->setLangType(L_USER, newName.c_str());
			}
			return TRUE;
		}

		case WM_CLOSE_USERDEFINE_DLG:
		{
			checkMenuItem(IDM_LANG_USER_DLG, false);
			_toolBar.setCheck(IDM_LANG_USER_DLG, false);
			return TRUE;
		}

		case WM_REPLACEALL_INOPENEDDOC:
		{
			replaceInOpenedFiles();
			return TRUE;
		}

		case WM_FINDALL_INOPENEDDOC:
		{
			findInOpenedFiles();
			return TRUE;
		}

		case WM_FINDALL_INCURRENTDOC:
		{
			return findInCurrentFile();
		}

		case WM_FINDINFILES:
		{
			return findInFiles();
		}

		case WM_FINDALL_INCURRENTFINDER:
		{
			FindersInfo *findInFolderInfo = reinterpret_cast<FindersInfo *>(wParam);
			Finder * newFinder = _findReplaceDlg.createFinder();
			
			findInFolderInfo->_pDestFinder = newFinder;
			bool isOK = findInFinderFiles(findInFolderInfo);
			return isOK;
		}

		case WM_REPLACEINFILES:
		{
			replaceInFiles();
			return TRUE;
		}

		case NPPM_LAUNCHFINDINFILESDLG:
		{
			// Find in files function code should be here due to the number of parameters (2) cannot be passed via WM_COMMAND
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];

			bool isFirstTime = not _findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL());

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
			_findReplaceDlg.launchFindInFilesDlg();
			setFindReplaceFolderFilter(reinterpret_cast<const TCHAR*>(wParam), reinterpret_cast<const TCHAR*>(lParam));

			return TRUE;
		}

		case NPPM_INTERNAL_FINDINFINDERDLG:
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];
			Finder *launcher = reinterpret_cast<Finder *>(wParam);

			bool isFirstTime = not _findInFinderDlg.isCreated();

			_findInFinderDlg.doDialog(launcher, _nativeLangSpeaker.isRTL());

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			setFindReplaceFolderFilter(NULL, NULL);

			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);

			return TRUE;
		}

		case NPPM_DOOPEN:
		case WM_DOOPEN:
		{
			BufferID id = doOpen(reinterpret_cast<const TCHAR *>(lParam));
			if (id != BUFFER_INVALID)
				return switchToFile(id);
			break;
		}

		case NPPM_INTERNAL_SETFILENAME:
		{
			if (!lParam && !wParam)
				return FALSE;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			if (b && b->getStatus() == DOC_UNNAMED) {
				b->setFileName(reinterpret_cast<const TCHAR*>(lParam));
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_GETBUFFERLANGTYPE:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getLangType();
		}

		case NPPM_SETBUFFERLANGTYPE:
		{
			if (!wParam)
				return FALSE;
			if (lParam < L_TEXT || lParam >= L_EXTERNAL || lParam == L_USER)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setLangType((LangType)lParam);
			return TRUE;
		}

		case NPPM_GETBUFFERENCODING:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getUnicodeMode();
		}

		case NPPM_SETBUFFERENCODING:
		{
			if (!wParam)
				return FALSE;
			if (lParam < uni8Bit || lParam >= uniEnd)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			if (b->getStatus() != DOC_UNNAMED || b->isDirty())	//do not allow to change the encoding if the file has any content
				return FALSE;
			b->setUnicodeMode((UniMode)lParam);
			return TRUE;
		}

		case NPPM_GETBUFFERFORMAT:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return static_cast<LRESULT>(b->getEolFormat());
		}

		case NPPM_SETBUFFERFORMAT:
		{
			if (!wParam)
				return FALSE;

			EolType newFormat = convertIntToFormatType(static_cast<int>(lParam), EolType::unknown);
			if (EolType::unknown == newFormat)
			{
				assert(false and "invalid buffer format message");
				return FALSE;
			}

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setEolFormat(newFormat);
			return TRUE;
		}

		case NPPM_GETBUFFERIDFROMPOS:
		{
			DocTabView* pView = nullptr;
			if (lParam == MAIN_VIEW)
				pView = &_mainDocTab;
			else if (lParam == SUB_VIEW)
				pView = &_subDocTab;
			else
				return reinterpret_cast<LRESULT>(BUFFER_INVALID);

			if ((size_t)wParam < pView->nbItem())
				return reinterpret_cast<LRESULT>(pView->getBufferByIndex(wParam));

			return reinterpret_cast<LRESULT>(BUFFER_INVALID);
		}

		case NPPM_GETCURRENTBUFFERID:
		{
			return reinterpret_cast<LRESULT>(_pEditView->getCurrentBufferID());
		}

		case NPPM_RELOADBUFFERID:
		{
			if (!wParam)
				return FALSE;
			return doReload(reinterpret_cast<BufferID>(wParam), lParam != 0);
		}

		case NPPM_RELOADFILE:
		{
			BufferID id = MainFileManager->getBufferFromName(reinterpret_cast<const TCHAR *>(lParam));
			if (id != BUFFER_INVALID)
				doReload(id, wParam != 0);
			break;
		}

		case NPPM_SWITCHTOFILE :
		{
			BufferID id = MainFileManager->getBufferFromName(reinterpret_cast<const TCHAR *>(lParam));
			if (id != BUFFER_INVALID)
				return switchToFile(id);
			return false;
		}

		case NPPM_SAVECURRENTFILE:
		{
			return fileSave();
		}

		case NPPM_SAVECURRENTFILEAS:
		{
			BufferID currentBufferID = _pEditView->getCurrentBufferID();
			bool asCopy = wParam == TRUE;
			const TCHAR *filename = reinterpret_cast<const TCHAR *>(lParam);
			if (!filename) return FALSE;
			return doSave(currentBufferID, filename, asCopy);
		}

		case NPPM_SAVEALLFILES:
		{
			return fileSaveAll();
		}

		case NPPM_SAVEFILE:
		{
			return fileSaveSpecific(reinterpret_cast<const TCHAR *>(lParam));
		}

		case NPPM_GETCURRENTNATIVELANGENCODING:
		{
			return _nativeLangSpeaker.getLangEncoding();
		}

		case NPPM_INTERNAL_DOCORDERCHANGED :
		{
			if (_pFileSwitcherPanel)
			{
				_pFileSwitcherPanel->updateTabOrder();
			}
			
			BufferID id = _pEditView->getCurrentBufferID();

			// Notify plugins that current file is about to be closed
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_DOCORDERCHANGED;
			scnN.nmhdr.hwndFrom = reinterpret_cast<void *>(lParam);
			scnN.nmhdr.idFrom = reinterpret_cast<uptr_t>(id);
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case NPPM_INTERNAL_EXPORTFUNCLISTANDQUIT:
		{
			checkMenuItem(IDM_VIEW_FUNC_LIST, true);
			_toolBar.setCheck(IDM_VIEW_FUNC_LIST, true);
			launchFunctionList();
			_pFuncList->setClosed(false);
			_pFuncList->serialize();

			::PostMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_FILE_EXIT, 0);
		}
		break;

		case NPPM_INTERNAL_PRNTANDQUIT:
		{
			::PostMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_FILE_PRINTNOW, 0);
			::PostMessage(_pPublicInterface->getHSelf(), WM_COMMAND, IDM_FILE_EXIT, 0);
		}
		break;

		case NPPM_DISABLEAUTOUPDATE:
		{
			NppGUI & nppGUI = const_cast<NppGUI &>(pNppParam->getNppGUI());
			nppGUI._autoUpdateOpt._doAutoUpdate = false;
			return TRUE;
		}

		case WM_SIZE:
		{
			RECT rc;
			_pPublicInterface->getClientRect(rc);
			if (lParam == 0)
				lParam = MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top);

			::MoveWindow(_rebarTop.getHSelf(), 0, 0, rc.right, _rebarTop.getHeight(), TRUE);
			_statusBar.adjustParts(rc.right);
			::SendMessage(_statusBar.getHSelf(), WM_SIZE, wParam, lParam);

			int rebarBottomHeight = _rebarBottom.getHeight();
			int statusBarHeight = _statusBar.getHeight();
			::MoveWindow(_rebarBottom.getHSelf(), 0, rc.bottom - rebarBottomHeight - statusBarHeight, rc.right, rebarBottomHeight, TRUE);

			getMainClientRect(rc);
			_dockingManager.reSizeTo(rc);

			if (_pDocMap)
			{
				_pDocMap->doMove();
				_pDocMap->reloadMap();
			}

			result = TRUE;
			break;
		}

		case WM_MOVE:
		{
			result = TRUE;
			break;
		}

		case WM_MOVING:
		{
			if (_pDocMap)
			{
				_pDocMap->doMove();
			}
			result = FALSE;
			break;
		}

		case WM_SIZING:
		{
			result = FALSE;
			break;
		}

		case WM_COPYDATA:
		{
			COPYDATASTRUCT *pCopyData = reinterpret_cast<COPYDATASTRUCT *>(lParam);

			switch (pCopyData->dwData)
			{
				case COPYDATA_PARAMS:
				{
					const CmdLineParamsDTO *cmdLineParam = static_cast<const CmdLineParamsDTO *>(pCopyData->lpData); // CmdLineParams object from another instance
					const DWORD cmdLineParamsSize = pCopyData->cbData;  // CmdLineParams size from another instance
					if (sizeof(CmdLineParamsDTO) == cmdLineParamsSize) // make sure the structure is the same
					{
						pNppParam->setCmdlineParam(*cmdLineParam);
					}
					else
					{
#ifdef DEBUG 
						printStr(TEXT("sizeof(CmdLineParams) != cmdLineParamsSize\rCmdLineParams is formed by an instance of another version,\rwhereas your CmdLineParams has been modified in this instance."));
#endif
					}

					NppGUI nppGui = (NppGUI)pNppParam->getNppGUI();
					nppGui._isCmdlineNosessionActivated = cmdLineParam->_isNoSession;
					break;
				}

				case COPYDATA_FILENAMESA:
				{
					char *fileNamesA = static_cast<char *>(pCopyData->lpData);
					const CmdLineParamsDTO & cmdLineParams = pNppParam->getCmdLineParams();
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *fileNamesW = wmc->char2wchar(fileNamesA, CP_ACP);
					loadCommandlineParams(fileNamesW, &cmdLineParams);
					break;
				}

				case COPYDATA_FILENAMESW:
				{
					wchar_t *fileNamesW = static_cast<wchar_t *>(pCopyData->lpData);
					const CmdLineParamsDTO & cmdLineParams = pNppParam->getCmdLineParams();
					loadCommandlineParams(fileNamesW, &cmdLineParams);
					break;
				}
			}

			return TRUE;
		}

		case WM_COMMAND:
		{
			if (HIWORD(wParam) == SCEN_SETFOCUS)
			{
				HWND hMain = _mainEditView.getHSelf(), hSec = _subEditView.getHSelf();
				HWND hFocus = reinterpret_cast<HWND>(lParam);
				if (hMain == hFocus)
					switchEditViewTo(MAIN_VIEW);
				else if (hSec == hFocus)
					switchEditViewTo(SUB_VIEW);
				else {
					//Other Scintilla, ignore
				}
				return TRUE;
			}
			else
			{
				command(LOWORD(wParam));
			}
			return TRUE;
		}

		case NPPM_INTERNAL_SAVECURRENTSESSION:
		{
			NppParameters *nppParam = NppParameters::getInstance();
			const NppGUI& nppGui = nppParam->getNppGUI();

			if (nppGui._rememberLastSession && !nppGui._isCmdlineNosessionActivated)
			{
				Session currentSession;
				getCurrentOpenedFiles(currentSession, true);
				nppParam->writeSession(currentSession);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_SAVEBACKUP:
		{
			if (NppParameters::getInstance()->getNppGUI().isSnapshotMode())
			{
				MainFileManager->backupCurrentBuffer();
			}

			return TRUE;
		}

		case NPPM_INTERNAL_RELOADNATIVELANG:
		{
			reloadLang();
			return TRUE;
		}

		case NPPM_INTERNAL_RELOADSTYLERS:
		{
			loadStyles();
			return TRUE;
		}

		case NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED:
		{
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_SHORTCUTREMAPPED;
			scnN.nmhdr.hwndFrom = reinterpret_cast<void *>(lParam); // ShortcutKey structure
			scnN.nmhdr.idFrom = (uptr_t)wParam; // cmdID
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case NPPM_GETSHORTCUTBYCMDID:
		{
			int cmdID = static_cast<int32_t>(wParam); // cmdID
			ShortcutKey *sk = reinterpret_cast<ShortcutKey *>(lParam); // ShortcutKey structure

			return _pluginsManager.getShortcutByCmdID(cmdID, sk);
		}

		case NPPM_MENUCOMMAND:
		{
			command(static_cast<int32_t>(lParam));
			return TRUE;
		}

		case NPPM_GETFULLCURRENTPATH:
		case NPPM_GETCURRENTDIRECTORY:
		case NPPM_GETFILENAME:
		case NPPM_GETNAMEPART:
		case NPPM_GETEXTPART:
		{
			TCHAR str[MAX_PATH];
			// par defaut : NPPM_GETCURRENTDIRECTORY
			wcscpy_s(str, _pEditView->getCurrentBuffer()->getFullPathName());
			TCHAR* fileStr = str;

			if (message == NPPM_GETCURRENTDIRECTORY)
				PathRemoveFileSpec(str);
			else if (message == NPPM_GETFILENAME)
				fileStr = PathFindFileName(str);
			else if (message == NPPM_GETNAMEPART)
			{
				fileStr = PathFindFileName(str);
				PathRemoveExtension(fileStr);
			}
			else if (message == NPPM_GETEXTPART)
				fileStr = PathFindExtension(str);

			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(fileStr) >= int(wParam))
				{
					return FALSE;
				}
			}

			lstrcpy(reinterpret_cast<TCHAR *>(lParam), fileStr);
			return TRUE;
		}

		case NPPM_GETCURRENTWORD:
		{
			const int strSize = CURRENTWORD_MAXLENGTH;
			TCHAR str[strSize];
			TCHAR *pTchar = reinterpret_cast<TCHAR *>(lParam);
			_pEditView->getGenericSelectedText(str, strSize);
			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))	//buffer too small
				{
					return FALSE;
				}
				else //buffer large enough, perform safe copy
				{
					lstrcpyn(pTchar, str, static_cast<int32_t>(wParam));
					return TRUE;
				}
			}

			lstrcpy(pTchar, str);
			return TRUE;
		}

		case NPPM_GETFILENAMEATCURSOR: // wParam = buffer length, lParam = (TCHAR*)buffer
		{
			const int strSize = CURRENTWORD_MAXLENGTH;
			TCHAR str[strSize];
			TCHAR strLine[strSize];
			size_t lineNumber;
			int col;
			int i;
			int hasSlash;
			TCHAR *pTchar = reinterpret_cast<TCHAR *>(lParam);

			_pEditView->getGenericSelectedText(str, strSize); // this is either the selected text, or the word under the cursor if there is no selection
			hasSlash = FALSE;
			for (i = 0; str[i] != 0; i++) if (CharacterIs(str[i], TEXT("\\/"))) hasSlash = TRUE;

			if (hasSlash == FALSE)
			{
				// it's not a full file name so try to find the beginning and ending of it
				int start;
				int end;
				const TCHAR *delimiters;

				lineNumber = _pEditView->getCurrentLineNumber();
				col = _pEditView->getCurrentColumnNumber();
				_pEditView->getLine(lineNumber, strLine, strSize);

				// find the start
				start = col;
				delimiters = TEXT(" \t[(\"<>");
				while ((start > 0) && (CharacterIs(strLine[start], delimiters) == FALSE)) start--;
				if (CharacterIs(strLine[start], delimiters)) start++;

				// find the end
				end = col;
				delimiters = TEXT(" \t:()[]<>\"\r\n");
				while ((strLine[end] != 0) && (CharacterIs(strLine[end], delimiters) == FALSE)) end++;

				lstrcpyn(str, &strLine[start], end - start + 1);
			}

			if (lstrlen(str) >= int(wParam))	//buffer too small
			{
				return FALSE;
			}
			else //buffer large enough, perform safe copy
			{
				lstrcpyn(pTchar, str, static_cast<int32_t>(wParam));
				return TRUE;
			}
		}

		case NPPM_GETNPPFULLFILEPATH:
		case NPPM_GETNPPDIRECTORY:
		{
			const int strSize = MAX_PATH;
			TCHAR str[strSize];

			::GetModuleFileName(NULL, str, strSize);

			if (message == NPPM_GETNPPDIRECTORY)
				PathRemoveFileSpec(str);

			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))
				{
					return FALSE;
				}
			}

			lstrcpy(reinterpret_cast<TCHAR *>(lParam), str);
			return TRUE;
		}

		case NPPM_GETCURRENTLINE:
		{
			return _pEditView->getCurrentLineNumber();
		}

		case NPPM_GETCURRENTCOLUMN:
		{
			return _pEditView->getCurrentColumnNumber();
		}

		case NPPM_GETCURRENTSCINTILLA:
		{
			int *id = reinterpret_cast<int *>(lParam);
			if (_pEditView == &_mainEditView)
				*id = MAIN_VIEW;
			else if (_pEditView == &_subEditView)
				*id = SUB_VIEW;
			else
				*id = -1;
			return TRUE;
		}

		case NPPM_GETCURRENTLANGTYPE:
		{
			*(reinterpret_cast<LangType *>(lParam)) = _pEditView->getCurrentBuffer()->getLangType();
			return TRUE;
		}

		case NPPM_SETCURRENTLANGTYPE:
		{
			_pEditView->getCurrentBuffer()->setLangType(static_cast<LangType>(lParam));
			return TRUE;
		}

		case NPPM_GETNBOPENFILES:
		{
			size_t nbDocPrimary = _mainDocTab.nbItem();
			size_t nbDocSecond = _subDocTab.nbItem();
			if (lParam == ALL_OPEN_FILES)
				return nbDocPrimary + nbDocSecond;
			else if (lParam == PRIMARY_VIEW)
				return nbDocPrimary;
			else if (lParam == SECOND_VIEW)
				return nbDocSecond;
		}

		case NPPM_GETOPENFILENAMESPRIMARY:
		case NPPM_GETOPENFILENAMESSECOND:
		case NPPM_GETOPENFILENAMES:
		{
			if (!wParam)
				return 0;

			TCHAR** fileNames = reinterpret_cast<TCHAR**>(wParam);
			size_t nbFileNames = static_cast<size_t>(lParam);

			size_t j = 0;
			if (message != NPPM_GETOPENFILENAMESSECOND)
			{
				for (size_t i = 0; i < _mainDocTab.nbItem() && j < nbFileNames; ++i)
				{
					BufferID id = _mainDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}

			if (message != NPPM_GETOPENFILENAMESPRIMARY)
			{
				for (size_t i = 0; i < _subDocTab.nbItem() && j < nbFileNames; ++i)
				{
					BufferID id = _subDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}
			return j;
		}

		case WM_GETTASKLISTINFO:
		{
			if (!wParam)
				return 0;

			TaskListInfo * tli = reinterpret_cast<TaskListInfo *>(wParam);
			getTaskListInfo(tli);

			if (lParam != 0)
			{
				for (size_t idx = 0; idx < tli->_tlfsLst.size(); ++idx)
				{
					if (tli->_tlfsLst[idx]._iView == currentView() &&
						tli->_tlfsLst[idx]._docIndex == _pDocTab->getCurrentTabIndex())
					{
						tli->_currentIndex = static_cast<int>(idx);
						break;
					}
				}
				return TRUE;
			}

			if (NppParameters::getInstance()->getNppGUI()._styleMRU)
			{
				tli->_currentIndex = 0;
				std::sort(tli->_tlfsLst.begin(),tli->_tlfsLst.end(),SortTaskListPred(_mainDocTab,_subDocTab));
			}
			else
			{
				for (size_t idx = 0; idx < tli->_tlfsLst.size(); ++idx)
				{
					if (tli->_tlfsLst[idx]._iView == currentView() &&
					    tli->_tlfsLst[idx]._docIndex == _pDocTab->getCurrentTabIndex())
					{
						tli->_currentIndex = static_cast<int>(idx);
						break;
					}
				}
			}
			return TRUE;
		}

		case WM_MOUSEWHEEL:
		{
			if (0 != (LOWORD(wParam) & MK_RBUTTON))
			{
				// redirect to the IDC_PREV_DOC or IDC_NEXT_DOC so that we have the unified process

				pNppParam->_isTaskListRBUTTONUP_Active = true;
				short zDelta = (short) HIWORD(wParam);
				return ::SendMessage(hwnd, WM_COMMAND, zDelta>0?IDC_PREV_DOC:IDC_NEXT_DOC, 0);
			}
			return TRUE;
		}

		case WM_APPCOMMAND:
		{
			switch(GET_APPCOMMAND_LPARAM(lParam))
			{
				case APPCOMMAND_BROWSER_BACKWARD:
				case APPCOMMAND_BROWSER_FORWARD:
				{
					size_t nbDoc = viewVisible(MAIN_VIEW) ? _mainDocTab.nbItem() : 0;
					nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;
					if (nbDoc > 1)
						activateNextDoc((GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)?dirDown:dirUp);
					_linkTriggered = true;
					break;
				}
			}
			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case NPPM_GETNBSESSIONFILES:
		{
			const TCHAR *sessionFileName = reinterpret_cast<const TCHAR *>(lParam);
			if ((!sessionFileName) || (sessionFileName[0] == '\0'))
				return 0;
			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
				return session2Load.nbMainFiles() + session2Load.nbSubFiles();
			return 0;
		}

		case NPPM_GETSESSIONFILES:
		{
			const TCHAR *sessionFileName = reinterpret_cast<const TCHAR *>(lParam);
			TCHAR **sessionFileArray = reinterpret_cast<TCHAR **>(wParam);

			if ((!sessionFileName) || (sessionFileName[0] == '\0'))
				return FALSE;

			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
			{
				size_t i = 0;
				for ( ; i < session2Load.nbMainFiles() ; )
				{
					const TCHAR *pFn = session2Load._mainViewFiles[i]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}

				for (size_t j = 0, len = session2Load.nbSubFiles(); j < len ; ++j)
				{
					const TCHAR *pFn = session2Load._subViewFiles[j]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_DECODESCI:
		{
			// convert to ASCII
			ScintillaEditView *pSci;
			if (wParam == MAIN_VIEW)
				pSci = &_mainEditView;
			else if (wParam == SUB_VIEW)
				pSci = &_subEditView;
			else
				return -1;

			// get text of current scintilla
			auto length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			char* buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, reinterpret_cast<LPARAM>(buffer));

			// convert here
			UniMode unicodeMode = pSci->getCurrentBuffer()->getUnicodeMode();
			Utf8_16_Write UnicodeConvertor;
			UnicodeConvertor.setEncoding(unicodeMode);
			length = UnicodeConvertor.convert(buffer, length-1);

			// set text in target
			pSci->execute(SCI_CLEARALL);
			pSci->addText(length, UnicodeConvertor.getNewBuf());
			pSci->execute(SCI_EMPTYUNDOBUFFER);

			pSci->execute(SCI_SETCODEPAGE);

			// set cursor position
			pSci->execute(SCI_GOTOPOS);

			// clean buffer
			delete [] buffer;

			return unicodeMode;
		}

		case NPPM_ENCODESCI:
		{
			// convert
			ScintillaEditView *pSci;
			if (wParam == MAIN_VIEW)
				pSci = &_mainEditView;
			else if (wParam == SUB_VIEW)
				pSci = &_subEditView;
			else
				return -1;

			// get text of current scintilla
			auto length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			char* buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, reinterpret_cast<LPARAM>(buffer));

			Utf8_16_Read UnicodeConvertor;
			length = UnicodeConvertor.convert(buffer, length-1);

			// set text in target
			pSci->execute(SCI_CLEARALL);
			pSci->addText(length, UnicodeConvertor.getNewBuf());

			pSci->execute(SCI_EMPTYUNDOBUFFER);

			// set cursor position
			pSci->execute(SCI_GOTOPOS);

			// clean buffer
			delete [] buffer;

			// set new encoding if BOM was changed by other programms
			UniMode um = UnicodeConvertor.getEncoding();
			(pSci->getCurrentBuffer())->setUnicodeMode(um);
			(pSci->getCurrentBuffer())->setDirty(true);
			return um;
		}

		case NPPM_ACTIVATEDOC:
		case NPPM_TRIGGERTABBARCONTEXTMENU:
		{
			// similar to NPPM_ACTIVEDOC
			int whichView = ((wParam != MAIN_VIEW) && (wParam != SUB_VIEW)) ? currentView() : static_cast<int32_t>(wParam);
			int index = static_cast<int32_t>(lParam);

			switchEditViewTo(whichView);
			activateDoc(index);

			if (message == NPPM_TRIGGERTABBARCONTEXTMENU)
			{
				// open here tab menu
				NMHDR	nmhdr;
				nmhdr.code = NM_RCLICK;

				nmhdr.hwndFrom = (whichView == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();

				nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
				::SendMessage(hwnd, WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
			}
			return TRUE;
		}

		case NPPM_GETNPPVERSION:
		{
			const TCHAR * verStr = VERSION_VALUE;
			TCHAR mainVerStr[16];
			TCHAR auxVerStr[16];
			bool isDot = false;
			int j =0;
			int k = 0;
			for (int i = 0 ; verStr[i] ; ++i)
			{
				if (verStr[i] == '.')
				{
					isDot = true;
				}
				else
				{
					if (!isDot)
						mainVerStr[j++] = verStr[i];
					else
						auxVerStr[k++] = verStr[i];
				}
			}

			mainVerStr[j] = '\0';
			auxVerStr[k] = '\0';

			int mainVer = 0, auxVer = 0;
			if (mainVerStr[0])
				mainVer = generic_atoi(mainVerStr);

			if (auxVerStr[0])
				auxVer = generic_atoi(auxVerStr);

			return MAKELONG(auxVer, mainVer);
		}

		case WM_GETCURRENTMACROSTATUS:
		{
			if (_recordingMacro)
				return MACRO_RECORDING_IN_PROGRESS;
			return (_macro.empty())?0:MACRO_RECORDING_HAS_STOPPED;
		}

		case WM_FRSAVE_INT:
		{
			_macro.push_back(recordedMacroStep(static_cast<int32_t>(wParam), 0, static_cast<long>(lParam), NULL, recordedMacroStep::mtSavedSnR));
			break;
		}

		case WM_FRSAVE_STR:
		{
			_macro.push_back(recordedMacroStep(static_cast<int32_t>(wParam), 0, 0, reinterpret_cast<const TCHAR *>(lParam), recordedMacroStep::mtSavedSnR));
			break;
		}

		case WM_MACRODLGRUNMACRO:
		{
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				int times = 1;
				if (_runMacroDlg.getMode() == RM_RUN_MULTI)
					times = _runMacroDlg.getTimes();
				else if (_runMacroDlg.getMode() == RM_RUN_EOF)
					times = -1;
				else
					break;

				int counter = 0;
				int lastLine = static_cast<int32_t>(_pEditView->execute(SCI_GETLINECOUNT)) - 1;
				int currLine = static_cast<int32_t>(_pEditView->getCurrentLineNumber());
				int indexMacro = _runMacroDlg.getMacro2Exec();
				int deltaLastLine = 0;
				int deltaCurrLine = 0;

				Macro m = _macro;

				if (indexMacro != -1)
				{
					vector<MacroShortcut> & ms = pNppParam->getMacroList();
					m = ms[indexMacro].getMacro();
				}

				_pEditView->execute(SCI_BEGINUNDOACTION);
				for (;;)
				{
					macroPlayback(m);
					++counter;
					if ( times >= 0 )
					{
						if ( counter >= times )
							break;
					}
					else // run until eof
					{
						bool cursorMovedUp = deltaCurrLine < 0;
						deltaLastLine = static_cast<int32_t>(_pEditView->execute(SCI_GETLINECOUNT)) - 1 - lastLine;
						deltaCurrLine = static_cast<int32_t>(_pEditView->getCurrentLineNumber()) - currLine;

						if (( deltaCurrLine == 0 )	// line no. not changed?
							&& (deltaLastLine >= 0))  // and no lines removed?
							break; // exit

						// Update the line count, but only if the number of lines remaining is shrinking.
						// Otherwise, the macro playback may never end.
						if (deltaLastLine < deltaCurrLine)
							lastLine += deltaLastLine;

						// save current line
						currLine += deltaCurrLine;

						// eof?
						if ((currLine >= lastLine) || (currLine < 0)
							|| ((deltaCurrLine == 0) && (currLine == 0) && ((deltaLastLine >= 0) || cursorMovedUp)))
						{
							break;
						}
					}
				}
				_pEditView->execute(SCI_ENDUNDOACTION);
			}
			break;
		}

		case NPPM_CREATESCINTILLAHANDLE:
		{
			return (LRESULT)_scintillaCtrls4Plugins.createSintilla((lParam == NULL?hwnd:reinterpret_cast<HWND>(lParam)));
		}

		case NPPM_INTERNAL_GETSCINTEDTVIEW:
		{
			return (LRESULT)_scintillaCtrls4Plugins.getScintillaEditViewFrom(reinterpret_cast<HWND>(lParam));
		}

		case NPPM_INTERNAL_ENABLESNAPSHOT:
		{
			launchDocumentBackupTask();
			return TRUE;
		}

		case NPPM_DESTROYSCINTILLAHANDLE:
		{
			return _scintillaCtrls4Plugins.destroyScintilla(reinterpret_cast<HWND>(lParam));
		}

		case NPPM_GETNBUSERLANG:
		{
			if (lParam)
				*(reinterpret_cast<int *>(lParam)) = IDM_LANG_USER;
			return pNppParam->getNbUserLang();
		}

		case NPPM_GETCURRENTDOCINDEX:
		{
			if (lParam == SUB_VIEW)
			{
				if (!viewVisible(SUB_VIEW))
					return -1;
				return _subDocTab.getCurrentTabIndex();
			}
			else //MAIN_VIEW
			{
				if (!viewVisible(MAIN_VIEW))
					return -1;
				return _mainDocTab.getCurrentTabIndex();
			}
		}

		case NPPM_SETSTATUSBAR:
		{
			TCHAR *str2set = reinterpret_cast<TCHAR *>(lParam);
			if (!str2set || !str2set[0])
				return FALSE;

			switch (wParam)
			{
				case STATUSBAR_DOC_TYPE:
				case STATUSBAR_DOC_SIZE:
				case STATUSBAR_CUR_POS:
				case STATUSBAR_EOF_FORMAT:
				case STATUSBAR_UNICODE_TYPE:
				case STATUSBAR_TYPING_MODE:
					_statusBar.setText(str2set, static_cast<int32_t>(wParam));
					return TRUE;
				default :
					return FALSE;
			}
		}

		case NPPM_GETMENUHANDLE:
		{
			if (wParam == NPPPLUGINMENU)
				return (LRESULT)_pluginsManager.getMenuHandle();
			else if (wParam == NPPMAINMENU)
				return (LRESULT)_mainMenuHandle;
			else
				return NULL;
		}

		case NPPM_LOADSESSION:
		{
			fileLoadSession(reinterpret_cast<const TCHAR *>(lParam));
			return TRUE;
		}

		case NPPM_SAVECURRENTSESSION:
		{
			return (LRESULT)fileSaveSession(0, NULL, reinterpret_cast<const TCHAR *>(lParam));
		}

		case NPPM_SAVESESSION:
		{
			sessionInfo *pSi = reinterpret_cast<sessionInfo *>(lParam);
			return (LRESULT)fileSaveSession(pSi->nbFile, pSi->files, pSi->sessionFilePathName);
		}

		case NPPM_INTERNAL_CLEARSCINTILLAKEY:
		{
			_mainEditView.execute(SCI_CLEARCMDKEY, wParam);
			_subEditView.execute(SCI_CLEARCMDKEY, wParam);
			return TRUE;
		}
		case NPPM_INTERNAL_BINDSCINTILLAKEY:
		{
			_mainEditView.execute(SCI_ASSIGNCMDKEY, wParam, lParam);
			_subEditView.execute(SCI_ASSIGNCMDKEY, wParam, lParam);
			return TRUE;
		}

		case NPPM_INTERNAL_CMDLIST_MODIFIED:
		{
			::DrawMenuBar(hwnd);
			return TRUE;
		}

		case NPPM_INTERNAL_MACROLIST_MODIFIED:
		{
			return TRUE;
		}

		case NPPM_INTERNAL_USERCMDLIST_MODIFIED:
		{
			return TRUE;
		}

		case NPPM_INTERNAL_FINDKEYCONFLICTS:
		{
			if (not wParam || not lParam) // Clean up current session
			{
				if (_pShortcutMapper != nullptr)
				{
					delete _pShortcutMapper;
					_pShortcutMapper = nullptr;
				}
				return TRUE;
			}

			if (_pShortcutMapper == nullptr) // Begin new session
			{
				_pShortcutMapper = new ShortcutMapper;
				if (_pShortcutMapper == nullptr)
					break;
			}

			*reinterpret_cast<bool*>(lParam) = _pShortcutMapper->findKeyConflicts(nullptr, *reinterpret_cast<KeyCombo*>(wParam), (size_t)-1);

			return TRUE;
		}

		case NPPM_INTERNAL_SETCARETWIDTH:
		{
			const NppGUI & nppGUI = pNppParam->getNppGUI();

			if (nppGUI._caretWidth < 4)
			{
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
				_mainEditView.execute(SCI_SETCARETWIDTH, nppGUI._caretWidth);
				_subEditView.execute(SCI_SETCARETWIDTH, nppGUI._caretWidth);
			}
			else
			{
				_mainEditView.execute(SCI_SETCARETWIDTH, 1);
				_subEditView.execute(SCI_SETCARETWIDTH, 1);
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
			}
			return TRUE;
		}

		case NPPM_SETSMOOTHFONT:
		{
			int param = (lParam == 0 ? SC_EFF_QUALITY_DEFAULT : SC_EFF_QUALITY_LCD_OPTIMIZED);
			_mainEditView.execute(SCI_SETFONTQUALITY, param);
			_subEditView.execute(SCI_SETFONTQUALITY, param);
			return TRUE;
		}

		case NPPM_SETEDITORBORDEREDGE:
		{
			bool withBorderEdge = (lParam == 1);
			_mainEditView.setBorderEdge(withBorderEdge);
			_subEditView.setBorderEdge(withBorderEdge);
			return TRUE;
		}

		case NPPM_INTERNAL_SCROLLBEYONDLASTLINE:
		{
			const bool endAtLastLine = not (pNppParam->getSVP())._scrollBeyondLastLine;
			_mainEditView.execute(SCI_SETENDATLASTLINE, endAtLastLine);
			_subEditView.execute(SCI_SETENDATLASTLINE, endAtLastLine);
			return TRUE;
		}

		case NPPM_INTERNAL_SETWORDCHARS:
		{
			_mainEditView.setWordChars();
			_subEditView.setWordChars();
			return TRUE;
		}

		case NPPM_INTERNAL_SETMULTISELCTION:
		{
			const NppGUI & nppGUI = pNppParam->getNppGUI();
			_mainEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			_subEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			return TRUE;
		}

		case NPPM_INTERNAL_SETCARETBLINKRATE:
		{
			const NppGUI & nppGUI = pNppParam->getNppGUI();
			_mainEditView.execute(SCI_SETCARETPERIOD, nppGUI._caretBlinkRate);
			_subEditView.execute(SCI_SETCARETPERIOD, nppGUI._caretBlinkRate);
			return TRUE;
		}

		case NPPM_INTERNAL_ISTABBARREDUCED:
		{
			return _toReduceTabBar?TRUE:FALSE;
		}

		// ADD: success->hwnd; failure->NULL
		// REMOVE: success->NULL; failure->hwnd
		case NPPM_MODELESSDIALOG:
		{
			if (wParam == MODELESSDIALOGADD)
			{
				for (size_t i = 0, len = _hModelessDlgs.size() ; i < len ; ++i)
				{
					if (_hModelessDlgs[i] == reinterpret_cast<HWND>(lParam))
						return NULL;
				}

				_hModelessDlgs.push_back(reinterpret_cast<HWND>(lParam));
				return lParam;
			}
			else
			{
				if (wParam == MODELESSDIALOGREMOVE)
				{
					for (size_t i = 0, len = _hModelessDlgs.size(); i < len ; ++i)
					{
						if (_hModelessDlgs[i] == reinterpret_cast<HWND>(lParam))
						{
							vector<HWND>::iterator hDlg = _hModelessDlgs.begin() + i;
							_hModelessDlgs.erase(hDlg);
							return NULL;
						}
					}
					return lParam;
				}
			}
			return TRUE;
		}

		case WM_CONTEXTMENU:
		{
			if (pNppParam->_isTaskListRBUTTONUP_Active)
			{
				pNppParam->_isTaskListRBUTTONUP_Active = false;
			}
			else
			{
				if ((HWND(wParam) == _mainEditView.getHSelf()) || (HWND(wParam) == _subEditView.getHSelf()))
				{
					if ((HWND(wParam) == _mainEditView.getHSelf()))
						switchEditViewTo(MAIN_VIEW);
					else
						switchEditViewTo(SUB_VIEW);

					POINT p;
					::GetCursorPos(&p);
					ContextMenu scintillaContextmenu;
					std::vector<MenuItemUnit>& tmp = pNppParam->getContextMenuItems();
					scintillaContextmenu.create(hwnd, tmp, _mainMenuHandle);
					scintillaContextmenu.display(p);
					return TRUE;
				}
			}

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case WM_NOTIFY:
		{
			SCNotification *notification = reinterpret_cast<SCNotification *>(lParam);

			if (notification->nmhdr.code == SCN_UPDATEUI)
			{
				checkClipboard(); //6
				checkUndoState(); //4
			}

			if (wParam == LINKTRIGGERED)
				notification->wParam = LINKTRIGGERED;

			_pluginsManager.notify(notification);

			return notify(notification);
		}

		case WM_ACTIVATEAPP:
		{
			if (wParam == TRUE) // if npp is about to be activated
			{
				::PostMessage(hwnd, NPPM_INTERNAL_CHECKDOCSTATUS, 0, 0);
			}
			return FALSE;
		}

		case NPPM_INTERNAL_CHECKDOCSTATUS:
		{
			const NppGUI & nppgui = pNppParam->getNppGUI();
			if (nppgui._fileAutoDetection != cdDisabled)
			{
				bool bCheckOnlyCurrentBuffer = (nppgui._fileAutoDetection & cdEnabledNew) ? true : false;

				checkModifiedDocument(bCheckOnlyCurrentBuffer);
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_INTERNAL_RELOADSCROLLTOEND:
		{
			Buffer *buf = reinterpret_cast<Buffer *>(wParam);
			buf->reload();
			return TRUE;
		}

		case NPPM_INTERNAL_STOPMONITORING:
		{
			Buffer *buf = reinterpret_cast<Buffer *>(wParam);
			monitoringStartOrStopAndUpdateUI(buf, false);
			return TRUE;
		}

		case NPPM_GETPOSFROMBUFFERID:
		{
			int i;

			if (lParam == SUB_VIEW)
			{
				if ((i = _subDocTab.getIndexByBuffer((BufferID)wParam)) != -1)
				{
					long view = SUB_VIEW;
					view <<= 30;
					return view|i;
				}
				if ((i = _mainDocTab.getIndexByBuffer((BufferID)wParam)) != -1)
				{
					long view = MAIN_VIEW;
					view <<= 30;
					return view|i;
				}
			}
			else
			{
				if ((i = _mainDocTab.getIndexByBuffer((BufferID)wParam)) != -1)
				{
					long view = MAIN_VIEW;
					view <<= 30;
					return view|i;
				}
				if ((i = _subDocTab.getIndexByBuffer((BufferID)wParam)) != -1)
				{
					long view = SUB_VIEW;
					view <<= 30;
					return view|i;
				}
			}
			return -1;
		}

		case NPPM_GETFULLPATHFROMBUFFERID:
		{
			return MainFileManager->getFileNameFromBuffer(reinterpret_cast<BufferID>(wParam), reinterpret_cast<TCHAR *>(lParam));
		}

		case NPPM_INTERNAL_ENABLECHECKDOCOPT:
		{
			NppGUI& nppgui = const_cast<NppGUI&>((pNppParam->getNppGUI()));
			if (wParam == CHECKDOCOPT_NONE)
				nppgui._fileAutoDetection = cdDisabled;
			else if (wParam == CHECKDOCOPT_UPDATESILENTLY)
				nppgui._fileAutoDetection = (cdEnabledOld | cdAutoUpdate);
			else if (wParam == CHECKDOCOPT_UPDATEGO2END)
				nppgui._fileAutoDetection = (cdEnabledOld | cdGo2end);
			else if (wParam == (CHECKDOCOPT_UPDATESILENTLY | CHECKDOCOPT_UPDATEGO2END))
				nppgui._fileAutoDetection = (cdEnabledOld | cdGo2end | cdAutoUpdate);

			return TRUE;
		}

		case WM_ACTIVATE:
		{
			_pEditView->getFocus();
			return TRUE;
		}

		case WM_DROPFILES:
		{
			dropFiles(reinterpret_cast<HDROP>(wParam));
			return TRUE;
		}

		case WM_UPDATESCINTILLAS:
		{
			//reset styler for change in Stylers.xml
			_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
			_mainEditView.performGlobalStyles();

			_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());
			_subEditView.performGlobalStyles();

			_findReplaceDlg.updateFinderScintilla();

			drawTabbarColoursFromStylerArray();

			// Update default fg/bg colors in Parameters for both internal/plugins docking dialog
			StyleArray & globalStyles = (NppParameters::getInstance())->getGlobalStylers();
			int i = globalStyles.getStylerIndexByID(STYLE_DEFAULT);
			Style & style = globalStyles.getStyler(i);
			(NppParameters::getInstance())->setCurrentDefaultFgColor(style._fgColor);
			(NppParameters::getInstance())->setCurrentDefaultBgColor(style._bgColor);

			// Set default fg/bg colors on internal docking dialog
			if (_pFuncList)
			{
				_pFuncList->setBackgroundColor(style._bgColor);
				_pFuncList->setForegroundColor(style._fgColor);
			}

			if (_pAnsiCharPanel)
			{
				_pAnsiCharPanel->setBackgroundColor(style._bgColor);
				_pAnsiCharPanel->setForegroundColor(style._fgColor);
			}

			if (_pFileSwitcherPanel)
			{
				_pFileSwitcherPanel->setBackgroundColor(style._bgColor);
				_pFileSwitcherPanel->setForegroundColor(style._fgColor);
			}

			if (_pClipboardHistoryPanel)
			{
				_pClipboardHistoryPanel->setBackgroundColor(style._bgColor);
				_pClipboardHistoryPanel->setForegroundColor(style._fgColor);
				_pClipboardHistoryPanel->redraw(true);
			}

			if (_pProjectPanel_1)
			{
				_pProjectPanel_1->setBackgroundColor(style._bgColor);
				_pProjectPanel_1->setForegroundColor(style._fgColor);
			}

			if (_pProjectPanel_2)
			{
				_pProjectPanel_2->setBackgroundColor(style._bgColor);
				_pProjectPanel_2->setForegroundColor(style._fgColor);
			}

			if (_pProjectPanel_3)
			{
				_pProjectPanel_3->setBackgroundColor(style._bgColor);
				_pProjectPanel_3->setForegroundColor(style._fgColor);
			}

			if (_pFileBrowser)
			{
				_pFileBrowser->setBackgroundColor(style._bgColor);
				_pFileBrowser->setForegroundColor(style._fgColor);
			}

			if (_pDocMap)
				_pDocMap->setSyntaxHiliting();

			// Notify plugins of update to styles xml
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_WORDSTYLESUPDATED;
			scnN.nmhdr.hwndFrom = hwnd;
			scnN.nmhdr.idFrom = (uptr_t) _pEditView->getCurrentBufferID();
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
		{
			if (_pPublicInterface->isPrelaunch())
			{
				SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
			else
			{
				SCNotification scnN;
				scnN.nmhdr.code = NPPN_BEFORESHUTDOWN;
				scnN.nmhdr.hwndFrom = hwnd;
				scnN.nmhdr.idFrom = 0;
				_pluginsManager.notify(&scnN);

				if (_pTrayIco)
					_pTrayIco->doTrayIcon(REMOVE);

				const NppGUI & nppgui = pNppParam->getNppGUI();

				bool isSnapshotMode = nppgui.isSnapshotMode();

				if (isSnapshotMode)
				{
					::LockWindowUpdate(hwnd);
					MainFileManager->backupCurrentBuffer();
				}

				Session currentSession;
				if (nppgui._rememberLastSession)
				{
					getCurrentOpenedFiles(currentSession, true);
					//Lock the recent file list so it isnt populated with opened files
					//Causing them to show on restart even though they are loaded by session
					_lastRecentFileList.setLock(true);	//only lock when the session is remembered
				}
				_isAttemptingCloseOnQuit = true;
				bool allClosed = fileCloseAll(false, isSnapshotMode);	//try closing files before doing anything else
				_isAttemptingCloseOnQuit = false;

				if (nppgui._rememberLastSession)
					_lastRecentFileList.setLock(false);	//only lock when the session is remembered

				if (!allClosed)
				{
					//User cancelled the shutdown
					scnN.nmhdr.code = NPPN_CANCELSHUTDOWN;
					_pluginsManager.notify(&scnN);
					
					if (isSnapshotMode)
						::LockWindowUpdate(NULL);
					return FALSE;
				}

				if (_beforeSpecialView.isFullScreen)	//closing, return to windowed mode
					fullScreenToggle();
				if (_beforeSpecialView.isPostIt)		//closing, return to windowed mode
					postItToggle();

				if (_configStyleDlg.isCreated() && ::IsWindowVisible(_configStyleDlg.getHSelf()))
					_configStyleDlg.restoreGlobalOverrideValues();

				scnN.nmhdr.code = NPPN_SHUTDOWN;
				_pluginsManager.notify(&scnN);


				saveScintillasZoom(); 
				saveGUIParams(); //writeGUIParams writeScintillaParams
				saveFindHistory(); //writeFindHistory
				_lastRecentFileList.saveLRFL(); //writeRecentFileHistorySettings, writeHistory
				saveProjectPanelsParams(); //writeProjectPanelsSettings
				saveFileBrowserParam();
				//
				// saving config.xml
				//
				pNppParam->saveConfig_xml();

				//
				// saving userDefineLang.xml
				//
				saveUserDefineLangs();

				//
				// saving shortcuts.xml
				//
				saveShortcuts();

				//
				// saving session.xml
				//
				if (nppgui._rememberLastSession && !nppgui._isCmdlineNosessionActivated)
					saveSession(currentSession);

				// write settings on cloud if enabled, if the settings files don't exist
				if (nppgui._cloudPath != TEXT("") && pNppParam->isCloudPathChanged())
				{
					bool isOK = pNppParam->writeSettingsFilesOnCloudForThe1stTime(nppgui._cloudPath);
					if (!isOK)
					{
						_nativeLangSpeaker.messageBox("SettingsOnCloudError",
							hwnd,
							TEXT("It seems the path of settings on cloud is set on a read only drive,\ror on a folder needed privilege right for writting access.\rYour settings on cloud will be canceled. Please reset a coherent value via Preference dialog."),
							TEXT("Settings on Cloud"),
							MB_OK | MB_APPLMODAL);
						pNppParam->removeCloudChoice();
					}
				}

				if (isSnapshotMode)
					::LockWindowUpdate(NULL);

				//Sends WM_DESTROY, Notepad++ will end
				if (message == WM_CLOSE)
					::DestroyWindow(hwnd);

				generic_string updaterFullPath = pNppParam->getWingupFullPath();
				if (!updaterFullPath.empty())
				{
					Process updater(updaterFullPath.c_str(), pNppParam->getWingupParams().c_str(), pNppParam->getWingupDir().c_str());
					updater.run(pNppParam->shouldDoUAC());
				}
			}
			return TRUE;
		}

		case WM_ENDSESSION:
		{
			if (wParam == TRUE)
				::DestroyWindow(hwnd);
			return 0;
		}

		case WM_DESTROY:
		{
			killAllChildren();
			::PostQuitMessage(0);
			_pPublicInterface->gNppHWND = NULL;
			return TRUE;
		}

		case WM_SYSCOMMAND:
		{
			const NppGUI & nppgui = (pNppParam->getNppGUI());
			if (((nppgui._isMinimizedToTray && !_isAdministrator) || _pPublicInterface->isPrelaunch()) && (wParam == SC_MINIMIZE))
			{
				if (nullptr == _pTrayIco)
					_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

				_pTrayIco->doTrayIcon(ADD);
				::ShowWindow(hwnd, SW_HIDE);
				return TRUE;
			}

			if (wParam == SC_KEYMENU && lParam == VK_SPACE)
			{
				_sysMenuEntering = true;
			}
			else if (wParam == 0xF093) //it should be SC_MOUSEMENU. A bug?
			{
				_sysMenuEntering = true;
			}

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case WM_LBUTTONDBLCLK:
		{
			::SendMessage(hwnd, WM_COMMAND, IDM_FILE_NEW, 0);
			return TRUE;
		}

		case IDC_MINIMIZED_TRAY:
		{
			switch (lParam)
			{
				//case WM_LBUTTONDBLCLK:
				case WM_LBUTTONUP :
				{
					_pEditView->getFocus();
					::ShowWindow(hwnd, SW_SHOW);
					if (!_pPublicInterface->isPrelaunch())
						_pTrayIco->doTrayIcon(REMOVE);
					::SendMessage(hwnd, WM_SIZE, 0, 0);
					return TRUE;
				}

				case WM_MBUTTONUP:
				{
					command(IDM_SYSTRAYPOPUP_NEW_AND_PASTE);
					return TRUE;
				}

				case WM_RBUTTONUP:
				{
					POINT p;
					GetCursorPos(&p);

					HMENU hmenu;            // menu template
					HMENU hTrayIconMenu;  // shortcut menu
					hmenu = ::LoadMenu(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDR_SYSTRAYPOPUP_MENU));
					hTrayIconMenu = ::GetSubMenu(hmenu, 0);
					SetForegroundWindow(hwnd);
					TrackPopupMenu(hTrayIconMenu, TPM_LEFTALIGN, p.x, p.y, 0, hwnd, NULL);
					PostMessage(hwnd, WM_NULL, 0, 0);
					DestroyMenu(hmenu);
					return TRUE;
				}
			}
			return TRUE;
		}

		case NPPM_DMMSHOW:
		{
			_dockingManager.showDockableDlg(reinterpret_cast<HWND>(lParam), SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMHIDE:
		{
			_dockingManager.showDockableDlg(reinterpret_cast<HWND>(lParam), SW_HIDE);
			return TRUE;
		}

		case NPPM_DMMUPDATEDISPINFO:
		{
			if (::IsWindowVisible(reinterpret_cast<HWND>(lParam)))
				_dockingManager.updateContainerInfo(reinterpret_cast<HWND>(lParam));
			return TRUE;
		}

		case NPPM_DMMREGASDCKDLG:
		{
			tTbData *pData = reinterpret_cast<tTbData *>(lParam);
			int		iCont	= -1;
			bool	isVisible	= false;

			getIntegralDockingData(*pData, iCont, isVisible);
			_dockingManager.createDockableDlg(*pData, iCont, isVisible);
			return TRUE;
		}

		case NPPM_DMMVIEWOTHERTAB:
		{
			_dockingManager.showDockableDlg(reinterpret_cast<TCHAR *>(lParam), SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMGETPLUGINHWNDBYNAME : //(const TCHAR *windowName, const TCHAR *moduleName)
		{
			if (!lParam)
				return NULL;

			TCHAR *moduleName = reinterpret_cast<TCHAR *>(lParam);
			TCHAR *windowName = reinterpret_cast<TCHAR *>(wParam);
			std::vector<DockingCont *> dockContainer = _dockingManager.getContainerInfo();

			for (size_t i = 0, len = dockContainer.size(); i < len ; ++i)
			{
				std::vector<tTbData *> tbData = dockContainer[i]->getDataOfAllTb();
				for (size_t j = 0, len2 = tbData.size() ; j < len2 ; ++j)
				{
					if (generic_stricmp(moduleName, tbData[j]->pszModuleName) == 0)
					{
						if (!windowName)
							return (LRESULT)tbData[j]->hClient;

						if (generic_stricmp(windowName, tbData[j]->pszName) == 0)
							return (LRESULT)tbData[j]->hClient;
					}
				}
			}
			return NULL;
		}

		case NPPM_ADDTOOLBARICON:
		{
			_toolBar.registerDynBtn(static_cast<UINT>(wParam), reinterpret_cast<toolbarIcons*>(lParam));
			return TRUE;
		}

		case NPPM_SETMENUITEMCHECK:
		{
			::CheckMenuItem(_mainMenuHandle, static_cast<UINT>(wParam), MF_BYCOMMAND | (static_cast<BOOL>(lParam) ? MF_CHECKED : MF_UNCHECKED));
			_toolBar.setCheck(static_cast<int>(wParam), lParam != 0);
			return TRUE;
		}

		case NPPM_GETWINDOWSVERSION:
		{
			return (NppParameters::getInstance())->getWinVersion();
		}

		case NPPM_MAKECURRENTBUFFERDIRTY:
		{
			_pEditView->getCurrentBuffer()->setDirty(true);
			return TRUE;
		}

		case NPPM_GETENABLETHEMETEXTUREFUNC:
		{
			return (LRESULT)pNppParam->getEnableThemeDlgTexture();
		}

		case NPPM_GETPLUGINSCONFIGDIR:
		{
			generic_string userPluginConfDir = pNppParam->getUserPluginConfDir();
			if (lParam != 0)
			{
				if (userPluginConfDir.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<TCHAR *>(lParam), userPluginConfDir.c_str());

				// For the retro-compatibility
				return TRUE;
			}
			return userPluginConfDir.length();
		}

		case NPPM_GETPLUGINHOMEPATH:
		{
			generic_string pluginHomePath = pNppParam->getPluginRootDir();
			if (lParam != 0)
			{
				if (pluginHomePath.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<TCHAR *>(lParam), pluginHomePath.c_str());
			}
			return pluginHomePath.length();
		}

		case NPPM_MSGTOPLUGIN :
		{
			return _pluginsManager.relayPluginMessages(message, wParam, lParam);
		}

		case NPPM_ALLOCATESUPPORTED:
		{
			return TRUE;
		}

		case NPPM_ALLOCATECMDID:
		{
			return _pluginsManager.allocateCmdID(static_cast<int32_t>(wParam), reinterpret_cast<int *>(lParam));
		}

		case NPPM_ALLOCATEMARKER:
		{
			return _pluginsManager.allocateMarker(static_cast<int32_t>(wParam), reinterpret_cast<int *>(lParam));
		}

		case NPPM_HIDETABBAR:
		{
			bool hide = (lParam != 0);
			bool oldVal = DocTabView::getHideTabBarStatus();
			if (hide == oldVal) return oldVal;

			DocTabView::setHideTabBarStatus(hide);
			::SendMessage(hwnd, WM_SIZE, 0, 0);

			NppGUI & nppGUI = const_cast<NppGUI &>(((NppParameters::getInstance())->getNppGUI()));
			if (hide)
				nppGUI._tabStatus |= TAB_HIDE;
			else
				nppGUI._tabStatus &= ~TAB_HIDE;

			return oldVal;
		}

		case NPPM_ISTABBARHIDDEN:
		{
			return _mainDocTab.getHideTabBarStatus();
		}

		case NPPM_HIDETOOLBAR:
		{
			bool show = (lParam != TRUE);
			bool currentStatus = _rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
			if (show != currentStatus)
				_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, show);
			return currentStatus;
		}

		case NPPM_ISTOOLBARHIDDEN :
		{
			return !_rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
		}

		case NPPM_HIDEMENU:
		{
			bool hide = (lParam == TRUE);
			bool isHidden = ::GetMenu(hwnd) == NULL;
			if (hide == isHidden)
				return isHidden;

			NppGUI & nppGUI = const_cast<NppGUI &>(pNppParam->getNppGUI());
			nppGUI._menuBarShow = !hide;
			if (nppGUI._menuBarShow)
				::SetMenu(hwnd, _mainMenuHandle);
			else
				::SetMenu(hwnd, NULL);

			return isHidden;
		}

		case NPPM_ISMENUHIDDEN:
		{
			return (::GetMenu(hwnd) == NULL);
		}

		case NPPM_HIDESTATUSBAR:
		{
			bool show = (lParam != TRUE);
			NppGUI & nppGUI = const_cast<NppGUI &>(pNppParam->getNppGUI());
			bool oldVal = nppGUI._statusBarShow;
			if (show == oldVal)
				return oldVal;

			RECT rc;
			_pPublicInterface->getClientRect(rc);

			nppGUI._statusBarShow = show;
			_statusBar.display(nppGUI._statusBarShow);
			::SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.bottom, rc.right));
			return oldVal;
		}

		case NPPM_ISSTATUSBARHIDDEN:
		{
			const NppGUI & nppGUI = pNppParam->getNppGUI();
			return !nppGUI._statusBarShow;
		}

		case NPPM_GETCURRENTVIEW:
		{
			return _activeView;
		}

		case NPPM_INTERNAL_ISFOCUSEDTAB:
		{
			HWND hTabToTest = (currentView() == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();
			return reinterpret_cast<HWND>(lParam) == hTabToTest;
		}

		case NPPM_INTERNAL_GETMENU:
		{
			return (LRESULT)_mainMenuHandle;
		}

		case NPPM_INTERNAL_CLEARINDICATOR:
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);
			return TRUE;
		}

		case NPPM_INTERNAL_CLEARINDICATORTAGMATCH:
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGMATCH);
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGATTR);
			return TRUE;
		}

		case NPPM_INTERNAL_CLEARINDICATORTAGATTR:
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGATTR);
			return TRUE;
		}

		case NPPM_INTERNAL_SWITCHVIEWFROMHWND:
		{
			HWND handle = reinterpret_cast<HWND>(lParam);
			if (_mainEditView.getHSelf() == handle || _mainDocTab.getHSelf() == handle)
			{
				switchEditViewTo(MAIN_VIEW);
			}
			else if (_subEditView.getHSelf() == handle || _subDocTab.getHSelf() == handle)
			{
				switchEditViewTo(SUB_VIEW);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_UPDATETITLEBAR:
		{
			setTitle();
			return TRUE;
		}

		case NPPM_INTERNAL_DISABLEAUTOUPDATE:
		{
			//printStr(TEXT("you've got me"));
			NppGUI & nppGUI = const_cast<NppGUI &>(pNppParam->getNppGUI());
			nppGUI._autoUpdateOpt._doAutoUpdate = false;
			return TRUE;
		}

		case NPPM_GETLANGUAGENAME:
		{
			generic_string langName = getLangDesc((LangType)wParam, true);
			if (lParam)
				lstrcpy((LPTSTR)lParam, langName.c_str());
			return langName.length();
		}

		case NPPM_GETLANGUAGEDESC:
		{
			generic_string langDesc = getLangDesc((LangType)wParam, false);
			if (lParam)
				lstrcpy((LPTSTR)lParam, langDesc.c_str());
			return langDesc.length();
		}

		case NPPM_DOCSWITCHERDISABLECOLUMN:
		{
			BOOL isOff = static_cast<BOOL>(lParam);
			NppGUI & nppGUI = const_cast<NppGUI &>(pNppParam->getNppGUI());
			nppGUI._fileSwitcherWithoutExtColumn = isOff == TRUE;

			if (_pFileSwitcherPanel)
			{
				_pFileSwitcherPanel->reload();
			}
			// else nothing to do
			return TRUE;
		}

		case NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR:
		case NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR:
		{
			return (message == NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR
					?(NppParameters::getInstance())->getCurrentDefaultFgColor()
					:(NppParameters::getInstance())->getCurrentDefaultBgColor());
		}

		case NPPM_SHOWDOCSWITCHER:
		{
			BOOL toShow = static_cast<BOOL>(lParam);
			if (toShow)
			{
				if (!_pFileSwitcherPanel || !_pFileSwitcherPanel->isVisible())
					launchFileSwitcherPanel();
			}
			else
			{
				if (_pFileSwitcherPanel)
					_pFileSwitcherPanel->display(false);
			}
			return TRUE;
		}

		case NPPM_ISDOCSWITCHERSHOWN:
		{
			if (!_pFileSwitcherPanel)
				return FALSE;
			return _pFileSwitcherPanel->isVisible();
		}

		// OLD BEHAVIOUR:
		// if doLocal, it's always false - having doLocal environment cannot load plugins outside
		// the presence of file "allowAppDataPlugins.xml" will be checked only when not doLocal
		//
		// NEW BEHAVIOUR:
		// No more file "allowAppDataPlugins.xml"
		// if doLocal - not allowed. Otherwise - allowed.
		case NPPM_GETAPPDATAPLUGINSALLOWED: 
		{
			const TCHAR *appDataNpp = pNppParam->getAppDataNppDir();
			if (appDataNpp[0]) // if not doLocal
			{
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_REMOVESHORTCUTBYCMDID:
		{
			int cmdID = static_cast<int32_t>(wParam);
			return _pluginsManager.removeShortcutByCmdID(cmdID);
		}

		//
		// These are sent by Preferences Dialog
		//
		case NPPM_INTERNAL_SETTING_HISTORY_SIZE:
		{
			_lastRecentFileList.setUserMaxNbLRF(pNppParam->getNbMaxRecentFile());
			break;
		}

		case NPPM_INTERNAL_SETTING_EDGE_SIZE:
		{
			ScintillaViewParams & svp = (ScintillaViewParams &)(NppParameters::getInstance())->getSVP();
			_mainEditView.execute(SCI_SETEDGECOLUMN, svp._edgeNbColumn);
			_subEditView.execute(SCI_SETEDGECOLUMN, svp._edgeNbColumn);
			break;
		}

		case NPPM_INTERNAL_SETTING_TAB_REPLCESPACE:
		case NPPM_INTERNAL_SETTING_TAB_SIZE:
		{
			_pEditView->setTabSettings(_pEditView->getCurrentBuffer()->getCurrentLang());
			break;
		}

		case NPPM_INTERNAL_RECENTFILELIST_UPDATE:
		{
			_lastRecentFileList.updateMenu();
			break;
		}

		case NPPM_INTERNAL_RECENTFILELIST_SWITCH:
		{
			_lastRecentFileList.switchMode();
			_lastRecentFileList.updateMenu();
			break;
		}

		case WM_INITMENUPOPUP:
		{
			_windowsMenu.initPopupMenu(reinterpret_cast<HMENU>(wParam), _pDocTab);
			return TRUE;
		}

		case WM_ENTERMENULOOP:
		{
			const NppGUI & nppgui = pNppParam->getNppGUI();
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(hwnd, _mainMenuHandle);

			return TRUE;
		}

		case WM_EXITMENULOOP:
		{
			const NppGUI & nppgui = pNppParam->getNppGUI();
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(hwnd, NULL);
			_sysMenuEntering = false;
			return FALSE;
		}

		case WM_DPICHANGED:
		{
			return TRUE;
		}

		default:
		{
			if (message == WDN_NOTIFY)
			{
				NMWINDLG* nmdlg = reinterpret_cast<NMWINDLG*>(lParam);
				switch (nmdlg->type)
				{
					case WDT_ACTIVATE:
					{
						activateDoc(nmdlg->curSel);
						nmdlg->processed = TRUE;
						break;
					}

					case WDT_SAVE:
					{
						//loop through nmdlg->nItems, get index and save it
						for (unsigned int i = 0; i < nmdlg->nItems; ++i)
						{
							fileSave(_pDocTab->getBufferByIndex(i));
						}
						nmdlg->processed = TRUE;
						break;
					}

					case WDT_CLOSE:
					{
						//loop through nmdlg->nItems, get index and close it
						for (unsigned int i = 0; i < nmdlg->nItems; ++i)
						{
							bool closed = fileClose(_pDocTab->getBufferByIndex(nmdlg->Items[i]), currentView());
							UINT pos = nmdlg->Items[i];
							// The window list only needs to be rearranged when the file was actually closed
							if (closed)
							{
								nmdlg->Items[i] = 0xFFFFFFFF; // indicate file was closed

								// Shift the remaining items downward to fill the gap
								for (unsigned int j = i + 1; j < nmdlg->nItems; ++j)
								{
									if (nmdlg->Items[j] > pos)
										nmdlg->Items[j]--;
								}
							}
						}
						nmdlg->processed = TRUE;
						break;
					}

					case WDT_SORT:
					{
						if (nmdlg->nItems != _pDocTab->nbItem())	//sanity check, if mismatch just abort
							break;

						//Collect all buffers
						std::vector<BufferID> tempBufs;
						for (unsigned int i = 0; i < nmdlg->nItems; ++i)
						{
							tempBufs.push_back(_pDocTab->getBufferByIndex(i));
						}
						//Reset buffers
						for (unsigned int i = 0; i < nmdlg->nItems; ++i)
						{
							_pDocTab->setBuffer(i, tempBufs[nmdlg->Items[i]]);
						}
						activateBuffer(_pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex()), currentView());
						break;
					}
				}
				return TRUE;
			}

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	_pluginsManager.relayNppMessages(message, wParam, lParam);
	return result;
}

