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


LRESULT CALLBACK Notepad_plus_Window::Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (hwnd == NULL)
		return FALSE;

	switch(Message)
	{
		case WM_NCCREATE:
		{
			// First message we get the ptr of instantiated object
			// then stock it into GWLP_USERDATA index in order to retrieve afterward
			Notepad_plus_Window *pM30ide = (Notepad_plus_Window *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
			pM30ide->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pM30ide);
			return TRUE;
		}

		default:
		{
			return ((Notepad_plus_Window *)::GetWindowLongPtr(hwnd, GWLP_USERDATA))->runProc(hwnd, Message, wParam, lParam);
		}
	}
}


LRESULT Notepad_plus_Window::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_CREATE:
		{
			try
			{
				_notepad_plus_plus_core._pPublicInterface = this;
				return _notepad_plus_plus_core.init(hwnd);
			}
			catch (std::exception ex)
			{
				::MessageBoxA(hwnd, ex.what(), "Exception On WM_CREATE", MB_OK);
				exit(-1);
			}
			break;
		}
		default:
		{
			if (this)
				return _notepad_plus_plus_core.process(hwnd, Message, wParam, lParam);
		}
	}
	return FALSE;
}


LRESULT Notepad_plus::process(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = FALSE;
	NppParameters *pNppParam = NppParameters::getInstance();

	switch (Message)
	{
		case WM_NCACTIVATE:
		{
			// Note: lParam is -1 to prevent endless loops of calls
			::SendMessage(_dockingManager.getHSelf(), WM_NCACTIVATE, wParam, (LPARAM)-1);
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
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
			TCHAR *userLangName = (TCHAR *)lParam;
			if (!userLangName || !userLangName[0])
				return FALSE;

			generic_string name{userLangName};

			//loop through buffers and reset the language (L_USER, TEXT("")) if (L_USER, name)
			for (int i = 0; i < MainFileManager->getNrBuffers(); ++i)
			{
				Buffer* buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && name == buf->getUserDefineLangName())
					buf->setLangType(L_USER, TEXT(""));
			}
			return TRUE;
		}

		case WM_RENAME_USERLANG:
		{
			if (!lParam || !(((TCHAR *)lParam)[0]) || !wParam || !(((TCHAR *)wParam)[0]))
				return FALSE;

			generic_string oldName{(TCHAR *)lParam};
			generic_string newName{(TCHAR *)wParam};

			//loop through buffers and reset the language (L_USER, newName) if (L_USER, oldName)
			for (int i = 0; i < MainFileManager->getNrBuffers(); ++i)
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
			findInCurrentFile();
			return TRUE;
		}

		case WM_FINDINFILES:
		{
			return findInFiles();
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
				_nativeLangSpeaker.changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
			_findReplaceDlg.launchFindInFilesDlg();
			setFindReplaceFolderFilter((const TCHAR*) wParam, (const TCHAR*) lParam);

			return TRUE;
		}

		case NPPM_DOOPEN:
		case WM_DOOPEN:
		{
			BufferID id = doOpen((const TCHAR *)lParam);
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
				b->setFileName((const TCHAR*)lParam);
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
			return static_cast<LRESULT>(b->getFormat());
		}

		case NPPM_SETBUFFERFORMAT:
		{
			if (!wParam)
				return FALSE;

			FormatType newFormat = convertIntToFormatType(static_cast<int>(lParam), FormatType::unknown);
			if (FormatType::unknown == newFormat)
			{
				assert(false and "invalid buffer format message");
				return FALSE;
			}

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setFormat(newFormat);
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
				return (LRESULT)BUFFER_INVALID;

			if ((int)wParam < pView->nbItem())
				return (LRESULT)(pView->getBufferByIndex((int)wParam));

			return (LRESULT)BUFFER_INVALID;
		}

		case NPPM_GETCURRENTBUFFERID:
		{
			return (LRESULT)(_pEditView->getCurrentBufferID());
		}

		case NPPM_RELOADBUFFERID:
		{
			if (!wParam)
				return FALSE;
			return doReload((BufferID)wParam, lParam != 0);
		}

		case NPPM_RELOADFILE:
		{
			BufferID id = MainFileManager->getBufferFromName((const TCHAR *)lParam);
			if (id != BUFFER_INVALID)
				doReload(id, wParam != 0);
			break;
		}

		case NPPM_SWITCHTOFILE :
		{
			BufferID id = MainFileManager->getBufferFromName((const TCHAR *)lParam);
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
			const TCHAR *filename = (const TCHAR *)lParam;
			if (!filename) return FALSE;
			return doSave(currentBufferID, filename, asCopy);
		}

		case NPPM_SAVEALLFILES:
		{
			return fileSaveAll();
		}

		case NPPM_GETCURRENTNATIVELANGENCODING:
		{
			return _nativeLangSpeaker.getLangEncoding();
		}

		case NPPM_INTERNAL_DOCORDERCHANGED :
		{
			BufferID id = _pEditView->getCurrentBufferID();

			// Notify plugins that current file is about to be closed
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_DOCORDERCHANGED;
			scnN.nmhdr.hwndFrom = (void *)lParam;
			scnN.nmhdr.idFrom = (uptr_t)id;
			_pluginsManager.notify(&scnN);
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
			COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT *)lParam;

			switch (pCopyData->dwData)
			{
				case COPYDATA_PARAMS:
				{
					CmdLineParams *cmdLineParam = (CmdLineParams *)pCopyData->lpData;
					pNppParam->setCmdlineParam(*cmdLineParam);
					NppGUI nppGui = (NppGUI)pNppParam->getNppGUI();
					nppGui._isCmdlineNosessionActivated = cmdLineParam->_isNoSession;
					break;
				}

				case COPYDATA_FILENAMESA:
				{
					char *fileNamesA = (char *)pCopyData->lpData;
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *fileNamesW = wmc->char2wchar(fileNamesA, CP_ACP);
					loadCommandlineParams(fileNamesW, &cmdLineParams);
					break;
				}

				case COPYDATA_FILENAMESW:
				{
					wchar_t *fileNamesW = (wchar_t *)pCopyData->lpData;
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
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
				HWND hFocus = (HWND)lParam;
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
			const NppGUI nppGui = nppParam->getNppGUI();

			if (nppGui._rememberLastSession && !nppGui._isCmdlineNosessionActivated)
			{
				Session currentSession;
				getCurrentOpenedFiles(currentSession, true);
				nppParam->writeSession(currentSession);
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
			scnN.nmhdr.hwndFrom = (void *)lParam; // ShortcutKey structure
			scnN.nmhdr.idFrom = (uptr_t)wParam; // cmdID
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case NPPM_GETSHORTCUTBYCMDID:
		{
			int cmdID = wParam; // cmdID
			ShortcutKey *sk = (ShortcutKey *)lParam; // ShortcutKey structure

			return _pluginsManager.getShortcutByCmdID(cmdID, sk);
		}

		case NPPM_MENUCOMMAND:
		{
			command(lParam);
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
			TCHAR *fileStr = lstrcpy(str, _pEditView->getCurrentBuffer()->getFullPathName());

			if (Message == NPPM_GETCURRENTDIRECTORY)
				PathRemoveFileSpec(str);
			else if (Message == NPPM_GETFILENAME)
				fileStr = PathFindFileName(str);
			else if (Message == NPPM_GETNAMEPART)
			{
				fileStr = PathFindFileName(str);
				PathRemoveExtension(fileStr);
			}
			else if (Message == NPPM_GETEXTPART)
				fileStr = PathFindExtension(str);

			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(fileStr) >= int(wParam))
				{
					::MessageBox(hwnd, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM error"), MB_OK);
					return FALSE;
				}
			}

			lstrcpy((TCHAR *)lParam, fileStr);
			return TRUE;
		}

		case NPPM_GETCURRENTWORD:
		{
			const int strSize = CURRENTWORD_MAXLENGTH;
			TCHAR str[strSize];

			_pEditView->getGenericSelectedText((TCHAR *)str, strSize);
			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))	//buffer too small
				{
					::MessageBox(hwnd, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM_GETCURRENTWORD error"), MB_OK);
					return FALSE;
				}
				else //buffer large enough, perform safe copy
				{
					lstrcpyn((TCHAR *)lParam, str, wParam);
					return TRUE;
				}
			}

			lstrcpy((TCHAR *)lParam, str);
			return TRUE;
		}

		case NPPM_GETNPPDIRECTORY:
		{
			const int strSize = MAX_PATH;
			TCHAR str[strSize];

			::GetModuleFileName(NULL, str, strSize);
			PathRemoveFileSpec(str);

			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))
				{
					::MessageBox(hwnd, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM_GETNPPDIRECTORY error"), MB_OK);
					return FALSE;
				}
			}

			lstrcpy((TCHAR *)lParam, str);
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
			if (_pEditView == &_mainEditView)
				*((int *)lParam) = MAIN_VIEW;
			else if (_pEditView == &_subEditView)
				*((int *)lParam) = SUB_VIEW;
			else
				*((int *)lParam) = -1;
			return TRUE;
		}

		case NPPM_GETCURRENTLANGTYPE:
		{
			*((LangType *)lParam) = _pEditView->getCurrentBuffer()->getLangType();
			return TRUE;
		}

		case NPPM_SETCURRENTLANGTYPE:
		{
			_pEditView->getCurrentBuffer()->setLangType((LangType)lParam);
			return TRUE;
		}

		case NPPM_GETNBOPENFILES:
		{
			int nbDocPrimary = _mainDocTab.nbItem();
			int nbDocSecond = _subDocTab.nbItem();
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

			TCHAR** fileNames = (TCHAR**) wParam;
			int nbFileNames = lParam;

			int j = 0;
			if (Message != NPPM_GETOPENFILENAMESSECOND)
			{
				for (int i = 0 ; i < _mainDocTab.nbItem() && j < nbFileNames ; ++i)
				{
					BufferID id = _mainDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}

			if (Message != NPPM_GETOPENFILENAMESPRIMARY)
			{
				for (int i = 0 ; i < _subDocTab.nbItem() && j < nbFileNames ; ++i)
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

			TaskListInfo * tli = (TaskListInfo *)wParam;
			getTaskListInfo(tli);

			if (lParam != 0)
			{
				for (int idx = 0; idx < (int)tli->_tlfsLst.size(); ++idx)
				{
					if (tli->_tlfsLst[idx]._iView == currentView() &&
						tli->_tlfsLst[idx]._docIndex == _pDocTab->getCurrentTabIndex())
					{
						tli->_currentIndex = idx;
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
				for (int idx = 0; idx < (int)tli->_tlfsLst.size(); ++idx)
				{
					if(tli->_tlfsLst[idx]._iView == currentView() &&
					   tli->_tlfsLst[idx]._docIndex == _pDocTab->getCurrentTabIndex())
					{
						tli->_currentIndex = idx;
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
					int nbDoc = viewVisible(MAIN_VIEW)?_mainDocTab.nbItem():0;
					nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;
					if (nbDoc > 1)
						activateNextDoc((GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)?dirDown:dirUp);
					_linkTriggered = true;
					break;
				}
			}
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case NPPM_GETNBSESSIONFILES:
		{
			const TCHAR *sessionFileName = (const TCHAR *)lParam;
			if ((!sessionFileName) || (sessionFileName[0] == '\0'))
				return 0;
			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
				return session2Load.nbMainFiles() + session2Load.nbSubFiles();
			return 0;
		}

		case NPPM_GETSESSIONFILES:
		{
			const TCHAR *sessionFileName = (const TCHAR *)lParam;
			TCHAR **sessionFileArray = (TCHAR **)wParam;

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
			UINT length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			char* buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

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
			UINT length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			char* buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

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
			int whichView = ((wParam != MAIN_VIEW) && (wParam != SUB_VIEW))?currentView():wParam;
			int index = lParam;

			switchEditViewTo(whichView);
			activateDoc(index);

			if (Message == NPPM_TRIGGERTABBARCONTEXTMENU)
			{
				// open here tab menu
				NMHDR	nmhdr;
				nmhdr.code = NM_RCLICK;

				nmhdr.hwndFrom = (whichView == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();

				nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
				::SendMessage(hwnd, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
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
			_macro.push_back(recordedMacroStep(wParam, 0, lParam, NULL, recordedMacroStep::mtSavedSnR));
			break;
		}

		case WM_FRSAVE_STR:
		{
			_macro.push_back(recordedMacroStep(wParam, 0, 0, (const TCHAR *)lParam, recordedMacroStep::mtSavedSnR));
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
				int lastLine = int(_pEditView->execute(SCI_GETLINECOUNT)) - 1;
				int currLine = _pEditView->getCurrentLineNumber();
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
						deltaLastLine = int(_pEditView->execute(SCI_GETLINECOUNT)) - 1 - lastLine;
						deltaCurrLine = _pEditView->getCurrentLineNumber() - currLine;

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
			return (LRESULT)_scintillaCtrls4Plugins.createSintilla((lParam == NULL?hwnd:(HWND)lParam));
		}

		case NPPM_INTERNAL_GETSCINTEDTVIEW:
		{
			return (LRESULT)_scintillaCtrls4Plugins.getScintillaEditViewFrom((HWND)lParam);
		}

		case NPPM_INTERNAL_ENABLESNAPSHOT:
		{
			launchDocumentBackupTask();
			return TRUE;
		}

		case NPPM_DESTROYSCINTILLAHANDLE:
		{
			return _scintillaCtrls4Plugins.destroyScintilla((HWND)lParam);
		}

		case NPPM_GETNBUSERLANG:
		{
			if (lParam)
				*((int *)lParam) = IDM_LANG_USER;
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
			TCHAR *str2set = (TCHAR *)lParam;
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
					_statusBar.setText(str2set, wParam);
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
			fileLoadSession((const TCHAR *)lParam);
			return TRUE;
		}

		case NPPM_SAVECURRENTSESSION:
		{
			return (LRESULT)fileSaveSession(0, NULL, (const TCHAR *)lParam);
		}

		case NPPM_SAVESESSION:
		{
			sessionInfo *pSi = (sessionInfo *)lParam;
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
			//changeMenuShortcut(lParam, (const TCHAR *)wParam);
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

		case NPPM_INTERNAL_SETCARETWIDTH:
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();

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

		case NPPM_INTERNAL_SETMULTISELCTION:
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			_mainEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			_subEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			return TRUE;
		}

		case NPPM_INTERNAL_SETCARETBLINKRATE:
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
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
					if (_hModelessDlgs[i] == (HWND)lParam)
						return NULL;
				}

				_hModelessDlgs.push_back((HWND)lParam);
				return lParam;
			}
			else
			{
				if (wParam == MODELESSDIALOGREMOVE)
				{
					for (size_t i = 0, len = _hModelessDlgs.size(); i < len ; ++i)
					{
						if (_hModelessDlgs[i] == (HWND)lParam)
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

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
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

		case NPPM_INTERNAL_CHECKDOCSTATUS:
		case WM_ACTIVATEAPP:
		{
			if (wParam == TRUE) // if npp is about to be activated
			{
				const NppGUI & nppgui = pNppParam->getNppGUI();
				if (LOWORD(wParam) && (nppgui._fileAutoDetection != cdDisabled))
				{
					_activeAppInf._isActivated = true;
					checkModifiedDocument();
					return FALSE;
				}
			}
			break;
		}

		case NPPM_INTERNAL_GETCHECKDOCOPT:
		{
			return (LRESULT)((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection;
		}

		case NPPM_INTERNAL_SETCHECKDOCOPT:
		{
			// If nothing is changed by user, then we allow to set this value
			if (((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection == ((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetectionOriginalValue)
				((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection = (ChangeDetect)wParam;
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
			return MainFileManager->getFileNameFromBuffer((BufferID)wParam, (TCHAR *)lParam);
		}

		case NPPM_INTERNAL_ENABLECHECKDOCOPT:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if (wParam == CHECKDOCOPT_NONE)
				nppgui._fileAutoDetection = cdDisabled;
			else if (wParam == CHECKDOCOPT_UPDATESILENTLY)
				nppgui._fileAutoDetection = cdAutoUpdate;
			else if (wParam == CHECKDOCOPT_UPDATEGO2END)
				nppgui._fileAutoDetection = cdGo2end;
			else if (wParam == (CHECKDOCOPT_UPDATESILENTLY | CHECKDOCOPT_UPDATEGO2END))
				nppgui._fileAutoDetection = cdAutoUpdateGo2end;

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
				bool allClosed = fileCloseAll(false, isSnapshotMode);	//try closing files before doing anything else

				if (nppgui._rememberLastSession)
					_lastRecentFileList.setLock(false);	//only lock when the session is remembered

				if (!allClosed)
				{
					//User cancelled the shutdown
					scnN.nmhdr.code = NPPN_CANCELSHUTDOWN;
					_pluginsManager.notify(&scnN);
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

				saveFindHistory(); //writeFindHistory
				_lastRecentFileList.saveLRFL(); //writeRecentFileHistorySettings, writeHistory
				saveScintillaParams(); //writeScintillaParams
				saveGUIParams(); //writeGUIParams
				saveProjectPanelsParams(); //writeProjectPanelsSettings
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
				if (Message == WM_CLOSE)
					::DestroyWindow(hwnd);
			}
			return TRUE;
		}

		case WM_ENDSESSION:
		{
			if(wParam == TRUE)
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
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if ((nppgui._isMinimizedToTray || _pPublicInterface->isPrelaunch()) && (wParam == SC_MINIMIZE))
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

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
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
			_dockingManager.showDockableDlg((HWND)lParam, SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMHIDE:
		{
			_dockingManager.showDockableDlg((HWND)lParam, SW_HIDE);
			return TRUE;
		}

		case NPPM_DMMUPDATEDISPINFO:
		{
			if (::IsWindowVisible((HWND)lParam))
				_dockingManager.updateContainerInfo((HWND)lParam);
			return TRUE;
		}

		case NPPM_DMMREGASDCKDLG:
		{
			tTbData *pData	= (tTbData *)lParam;
			int		iCont	= -1;
			bool	isVisible	= false;

			getIntegralDockingData(*pData, iCont, isVisible);
			_dockingManager.createDockableDlg(*pData, iCont, isVisible);
			return TRUE;
		}

		case NPPM_DMMVIEWOTHERTAB:
		{
			_dockingManager.showDockableDlg((TCHAR*)lParam, SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMGETPLUGINHWNDBYNAME : //(const TCHAR *windowName, const TCHAR *moduleName)
		{
			if (!lParam)
				return NULL;

			TCHAR *moduleName = (TCHAR *)lParam;
			TCHAR *windowName = (TCHAR *)wParam;
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
			_toolBar.registerDynBtn((UINT)wParam, (toolbarIcons*)lParam);
			return TRUE;
		}

		case NPPM_SETMENUITEMCHECK:
		{
			::CheckMenuItem(_mainMenuHandle, (UINT)wParam, MF_BYCOMMAND | ((BOOL)lParam ? MF_CHECKED : MF_UNCHECKED));
			_toolBar.setCheck((int)wParam, bool(lParam != 0));
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
			if (!lParam || !wParam)
				return FALSE;

			generic_string pluginsConfigDirPrefix = pNppParam->getAppDataNppDir();

			if (pluginsConfigDirPrefix == TEXT(""))
				pluginsConfigDirPrefix = pNppParam->getNppPath();

			const TCHAR *secondPart = TEXT("plugins\\Config");

			size_t len = wParam;
			if (len < pluginsConfigDirPrefix.length() + lstrlen(secondPart))
				return FALSE;

			TCHAR *pluginsConfigDir = (TCHAR *)lParam;
			lstrcpy(pluginsConfigDir, pluginsConfigDirPrefix.c_str());

			::PathAppend(pluginsConfigDir, secondPart);
			return TRUE;
		}

		case NPPM_MSGTOPLUGIN :
		{
			return _pluginsManager.relayPluginMessages(Message, wParam, lParam);
		}

		case NPPM_ALLOCATESUPPORTED:
		{
			return TRUE;
		}

		case NPPM_ALLOCATECMDID:
		{
			return _pluginsManager.allocateCmdID(wParam, reinterpret_cast<int *>(lParam));
		}

		case NPPM_ALLOCATEMARKER:
		{
			return _pluginsManager.allocateMarker(wParam, reinterpret_cast<int *>(lParam));
		}

		case NPPM_HIDETABBAR:
		{
			bool hide = (lParam != 0);
			bool oldVal = DocTabView::getHideTabBarStatus();
			if (hide == oldVal) return oldVal;

			DocTabView::setHideTabBarStatus(hide);
			::SendMessage(hwnd, WM_SIZE, 0, 0);

			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
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

			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
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
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
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
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			return !nppGUI._statusBarShow;
		}

		case NPPM_GETCURRENTVIEW:
		{
			return _activeView;
		}

		case NPPM_INTERNAL_ISFOCUSEDTAB:
		{
			HWND hTabToTest = (currentView() == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();
			return (HWND)lParam == hTabToTest;
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
			HWND handle = (HWND)lParam;
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
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
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
			BOOL isOff = lParam;
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
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
			return (Message == NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR
					?(NppParameters::getInstance())->getCurrentDefaultFgColor()
					:(NppParameters::getInstance())->getCurrentDefaultBgColor());
		}

		case NPPM_SHOWDOCSWITCHER:
		{
			BOOL toShow = lParam;
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

		case NPPM_GETAPPDATAPLUGINSALLOWED:
		{
			const TCHAR *appDataNpp = pNppParam->getAppDataNppDir();
			if (appDataNpp[0])
			{
				generic_string allowAppDataPluginsPath(pNppParam->getNppPath());
				PathAppend(allowAppDataPluginsPath, allowAppDataPluginsFile);
				return ::PathFileExists(allowAppDataPluginsPath.c_str());
			}
			return FALSE;
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
			_windowsMenu.initPopupMenu((HMENU)wParam, _pDocTab);
			return TRUE;
		}

		case WM_ENTERMENULOOP:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(hwnd, _mainMenuHandle);

			return TRUE;
		}

		case WM_EXITMENULOOP:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
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
			if (Message == WDN_NOTIFY)
			{
				NMWINDLG* nmdlg = (NMWINDLG*)lParam;
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
						for (int i = 0; i < (int)nmdlg->nItems; ++i)
						{
							fileSave(_pDocTab->getBufferByIndex(i));
						}
						nmdlg->processed = TRUE;
						break;
					}

					case WDT_CLOSE:
					{
						//loop through nmdlg->nItems, get index and close it
						for (int i = 0; i < (int)nmdlg->nItems; ++i)
						{
							bool closed = fileClose(_pDocTab->getBufferByIndex(nmdlg->Items[i]), currentView());
							UINT pos = nmdlg->Items[i];
							// The window list only needs to be rearranged when the file was actually closed
							if (closed)
							{
								nmdlg->Items[i] = 0xFFFFFFFF; // indicate file was closed

								// Shift the remaining items downward to fill the gap
								for (int j = i + 1; j < (int)nmdlg->nItems; ++j)
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
						if (nmdlg->nItems != (unsigned int)_pDocTab->nbItem())	//sanity check, if mismatch just abort
							break;

						//Collect all buffers
						std::vector<BufferID> tempBufs;
						for (int i = 0; i < (int)nmdlg->nItems; ++i)
						{
							tempBufs.push_back(_pDocTab->getBufferByIndex(i));
						}
						//Reset buffers
						for (int i = 0; i < (int)nmdlg->nItems; ++i)
						{
							_pDocTab->setBuffer(i, tempBufs[nmdlg->Items[i]]);
						}
						activateBuffer(_pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex()), currentView());
						break;
					}
				}
				return TRUE;
			}

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}
	}

	_pluginsManager.relayNppMessages(Message, wParam, lParam);
	return result;
}

