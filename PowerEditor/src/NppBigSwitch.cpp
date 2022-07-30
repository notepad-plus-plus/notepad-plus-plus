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
#include "NppDarkMode.h"

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
		Buffer * bufL = MainFileManager.getBufferByID(lID);
		Buffer * bufR = MainFileManager.getBufferByID(rID);
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

			if (NppDarkMode::isExperimentalSupported())
			{
				NppDarkMode::enableDarkScrollBarForWindowAndChildren(hwnd);
			}

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
				NppDarkMode::setDarkTitleBar(hwnd);

				_notepad_plus_plus_core._pPublicInterface = this;
				LRESULT lRet = _notepad_plus_plus_core.init(hwnd);

				if (NppDarkMode::isEnabled() && NppDarkMode::isExperimentalSupported())
				{
					RECT rcClient;
					GetWindowRect(hwnd, &rcClient);

					// Inform application of the frame change.
					SetWindowPos(hwnd,
						NULL,
						rcClient.left, rcClient.top,
						rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
						SWP_FRAMECHANGED);
				}

				return lRet;
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
	NppParameters& nppParam = NppParameters::getInstance();

	if (NppDarkMode::isDarkMenuEnabled() && NppDarkMode::isEnabled() && NppDarkMode::runUAHWndProc(hwnd, message, wParam, lParam, &result))
	{
		return result;
	}

	switch (message)
	{
		case WM_NCACTIVATE:
		{
			// Note: lParam is -1 to prevent endless loops of calls
			::SendMessage(_dockingManager.getHSelf(), WM_NCACTIVATE, wParam, -1);
			result = ::DefWindowProc(hwnd, message, wParam, lParam);
			if (NppDarkMode::isDarkMenuEnabled() && NppDarkMode::isEnabled())
			{
				NppDarkMode::drawUAHMenuNCBottomLine(hwnd);
			}

			NppDarkMode::calculateTreeViewStyle();
			return result;
		}

		case WM_NCPAINT:
		{
			result = ::DefWindowProc(hwnd, message, wParam, lParam);
			if (NppDarkMode::isDarkMenuEnabled() && NppDarkMode::isEnabled())
			{
				NppDarkMode::drawUAHMenuNCBottomLine(hwnd);
			}
			return result;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc = {};
				GetClientRect(hwnd, &rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
				return 0;
			}
			else
			{
				return ::DefWindowProc(hwnd, message, wParam, lParam);
			}
		}

		case WM_SETTINGCHANGE:
		{
			NppDarkMode::handleSettingChange(hwnd, lParam);

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			refreshDarkMode(static_cast<bool>(wParam));
			// Notify plugins that Dark Mode changed
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_DARKMODECHANGED;
			scnN.nmhdr.hwndFrom = reinterpret_cast<void*>(lParam);
			scnN.nmhdr.idFrom = 0;
			_pluginsManager.notify(&scnN);
			return TRUE;
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
			for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
			{
				Buffer* buf = MainFileManager.getBufferByIndex(i);
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
			for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
			{
				Buffer* buf = MainFileManager.getBufferByIndex(i);
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
			const bool isEntireDoc = wParam == 0;
			return findInCurrentFile(isEntireDoc);
		}

		case WM_FINDINFILES:
		{
			return findInFiles();
		}

		case WM_FINDINPROJECTS:
		{
			return findInProjects();
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

		case WM_REPLACEINPROJECTS:
		{
			replaceInProjects();
			return TRUE;
		}

		case NPPM_LAUNCHFINDINFILESDLG:
		{
			// Find in files function code should be here due to the number of parameters (2) cannot be passed via WM_COMMAND
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL());
			
			const NppGUI & nppGui = nppParam.getNppGUI();
			if (!nppGui._stopFillingFindField)
			{
				_pEditView->getGenericSelectedText(str, strSize);
				_findReplaceDlg.setSearchText(str);
			}

			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
			_findReplaceDlg.launchFindInFilesDlg();
			setFindReplaceFolderFilter(reinterpret_cast<const TCHAR*>(wParam), reinterpret_cast<const TCHAR*>(lParam));

			return TRUE;
		}

		case NPPM_INTERNAL_FINDINPROJECTS:
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];

			bool isFirstTime = not _findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL());

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			if (isFirstTime)
				_nativeLangSpeaker.changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
			_findReplaceDlg.launchFindInProjectsDlg();
			_findReplaceDlg.setProjectCheckmarks(NULL, (int) wParam);
			return TRUE;
		}

		case NPPM_INTERNAL_FINDINFINDERDLG:
		{
			const int strSize = FINDREPLACE_MAXLENGTH;
			TCHAR str[strSize];
			Finder *launcher = reinterpret_cast<Finder *>(wParam);

			bool isFirstTime = !_findInFinderDlg.isCreated();

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
			Buffer * b = MainFileManager.getBufferByID(id);
			if (b && b->getStatus() == DOC_UNNAMED)
			{
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
			Buffer * b = MainFileManager.getBufferByID(id);
			return b->getLangType();
		}

		case NPPM_SETBUFFERLANGTYPE:
		{
			if (!wParam)
				return FALSE;
			if (lParam < L_TEXT || lParam >= L_EXTERNAL || lParam == L_USER)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager.getBufferByID(id);
			b->setLangType((LangType)lParam);
			return TRUE;
		}

		case NPPM_GETBUFFERENCODING:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager.getBufferByID(id);
			return b->getUnicodeMode();
		}

		case NPPM_SETBUFFERENCODING:
		{
			if (!wParam)
				return FALSE;
			if (lParam < uni8Bit || lParam >= uniEnd)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager.getBufferByID(id);
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
			Buffer * b = MainFileManager.getBufferByID(id);
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
			Buffer * b = MainFileManager.getBufferByID(id);
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
			TCHAR longNameFullpath[MAX_PATH];
			const TCHAR* pFilePath = reinterpret_cast<const TCHAR*>(lParam);
			wcscpy_s(longNameFullpath, MAX_PATH, pFilePath);
			if (_tcschr(longNameFullpath, '~'))
			{
				::GetLongPathName(longNameFullpath, longNameFullpath, MAX_PATH);
			}

			BufferID id = MainFileManager.getBufferFromName(longNameFullpath);
			if (id != BUFFER_INVALID)
				doReload(id, wParam != 0);
			break;
		}

		case NPPM_SWITCHTOFILE :
		{
			BufferID id = MainFileManager.getBufferFromName(reinterpret_cast<const TCHAR *>(lParam));
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
			if (_pDocumentListPanel)
			{
				_pDocumentListPanel->updateTabOrder();
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
			NppGUI & nppGUI = nppParam.getNppGUI();
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
				case COPYDATA_FULL_CMDLINE:
				{
					nppParam.setCmdLineString(static_cast<wchar_t*>(pCopyData->lpData));
					break;
				}

				case COPYDATA_PARAMS:
				{
					const CmdLineParamsDTO *cmdLineParam = static_cast<const CmdLineParamsDTO *>(pCopyData->lpData); // CmdLineParams object from another instance
					const DWORD cmdLineParamsSize = pCopyData->cbData;  // CmdLineParams size from another instance
					if (sizeof(CmdLineParamsDTO) == cmdLineParamsSize) // make sure the structure is the same
					{
						nppParam.setCmdlineParam(*cmdLineParam);
						generic_string pluginMessage { nppParam.getCmdLineParams()._pluginMessage };
						if (!pluginMessage.empty())
						{
							SCNotification scnN;
							scnN.nmhdr.code = NPPN_CMDLINEPLUGINMSG;
							scnN.nmhdr.hwndFrom = hwnd;
							scnN.nmhdr.idFrom = reinterpret_cast<uptr_t>(pluginMessage.c_str());
							_pluginsManager.notify(&scnN);
						}
					}
					else
					{
#ifdef DEBUG 
						printStr(TEXT("sizeof(CmdLineParams) != cmdLineParamsSize\rCmdLineParams is formed by an instance of another version,\rwhereas your CmdLineParams has been modified in this instance."));
#endif
					}

					NppGUI nppGui = (NppGUI)nppParam.getNppGUI();
					nppGui._isCmdlineNosessionActivated = cmdLineParam->_isNoSession;
					break;
				}

				case COPYDATA_FILENAMESA:
				{
					char *fileNamesA = static_cast<char *>(pCopyData->lpData);
					const CmdLineParamsDTO & cmdLineParams = nppParam.getCmdLineParams();
					WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *fileNamesW = wmc.char2wchar(fileNamesA, CP_ACP);
					loadCommandlineParams(fileNamesW, &cmdLineParams);
					break;
				}

				case COPYDATA_FILENAMESW:
				{
					wchar_t *fileNamesW = static_cast<wchar_t *>(pCopyData->lpData);
					const CmdLineParamsDTO & cmdLineParams = nppParam.getCmdLineParams();
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
				else
				{
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
			const NppGUI& nppGui = nppParam.getNppGUI();

			if (nppGui._rememberLastSession && !nppGui._isCmdlineNosessionActivated)
			{
				Session currentSession;
				getCurrentOpenedFiles(currentSession, true);
				nppParam.writeSession(currentSession);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_SAVEBACKUP:
		{
			if (NppParameters::getInstance().getNppGUI().isSnapshotMode())
			{
				MainFileManager.backupCurrentBuffer();
			}

			return TRUE;
		}

		case NPPM_INTERNAL_CHANGETABBAEICONS:
		{
			_mainDocTab.changeIcons(static_cast<unsigned char>(lParam));
			_subDocTab.changeIcons(static_cast<unsigned char>(lParam));

			//restart document list with the same icons as the DocTabs
			if (_pDocumentListPanel)
			{
				if (!_pDocumentListPanel->isClosed()) // if doclist is open
				{
					//close the doclist
					_pDocumentListPanel->display(false);

					//clean doclist
					_pDocumentListPanel->destroy();
					_pDocumentListPanel = nullptr;

					//relaunch with new icons
					launchDocumentListPanel();
				}
				else //if doclist is closed
				{
					//clean doclist
					_pDocumentListPanel->destroy();
					_pDocumentListPanel = nullptr;

					//relaunch doclist with new icons and close it
					launchDocumentListPanel();
					if (_pDocumentListPanel)
					{
						_pDocumentListPanel->display(false);
						_pDocumentListPanel->setClosed(true);
						checkMenuItem(IDM_VIEW_DOCLIST, false);
						_toolBar.setCheck(IDM_VIEW_DOCLIST, false);
					}
				}
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
			TCHAR str[MAX_PATH] = { '\0' };
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
		case NPPM_GETCURRENTLINESTR:
		{
			const int strSize = CURRENTWORD_MAXLENGTH;
			TCHAR str[strSize] = { '\0' };
			TCHAR *pTchar = reinterpret_cast<TCHAR *>(lParam);

			if (message == NPPM_GETCURRENTWORD)
				_pEditView->getGenericSelectedText(str, strSize);
			else if (message == NPPM_GETCURRENTLINESTR)
				_pEditView->getLine(_pEditView->getCurrentLineNumber(), str, strSize);

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
			intptr_t col;
			int hasSlash;
			TCHAR *pTchar = reinterpret_cast<TCHAR *>(lParam);

			_pEditView->getGenericSelectedText(str, strSize); // this is either the selected text, or the word under the cursor if there is no selection
			hasSlash = FALSE;
			for (int i = 0; str[i] != 0; i++)
				if (CharacterIs(str[i], TEXT("\\/")))
					hasSlash = TRUE;

			if (hasSlash == FALSE)
			{
				// it's not a full file name so try to find the beginning and ending of it
				intptr_t start;
				intptr_t end;
				const TCHAR *delimiters;

				lineNumber = _pEditView->getCurrentLineNumber();
				col = _pEditView->getCurrentColumnNumber();
				_pEditView->getLine(lineNumber, strLine, strSize);

				// find the start
				start = col;
				delimiters = TEXT(" \t[(\"<>");
				while ((start > 0) && (CharacterIs(strLine[start], delimiters) == FALSE))
					start--;

				if (CharacterIs(strLine[start], delimiters)) start++;

				// find the end
				end = col;
				delimiters = TEXT(" \t:()[]<>\"\r\n");
				while ((strLine[end] != 0) && (CharacterIs(strLine[end], delimiters) == FALSE)) end++;

				lstrcpyn(str, &strLine[start], static_cast<int>(end - start + 1));
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
					Buffer * buf = MainFileManager.getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}

			if (message != NPPM_GETOPENFILENAMESPRIMARY)
			{
				for (size_t i = 0; i < _subDocTab.nbItem() && j < nbFileNames; ++i)
				{
					BufferID id = _subDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager.getBufferByID(id);
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

			if (NppParameters::getInstance().getNppGUI()._styleMRU)
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

				nppParam._isTaskListRBUTTONUP_Active = true;
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
			if (nppParam.loadSession(session2Load, sessionFileName))
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
			if (nppParam.loadSession(session2Load, sessionFileName))
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

		// ADD_ZERO_PADDING == TRUE
		// 
		// version  | HIWORD | LOWORD
		//------------------------------
		// 8.9.6.4  | 8      | 964
		// 9        | 9      | 0
		// 6.9      | 6      | 900
		// 6.6.6    | 6      | 660
		// 13.6.6.6 | 13     | 666
		// 
		// 
		// ADD_ZERO_PADDING == FALSE
		// 
		// version  | HIWORD | LOWORD
		//------------------------------
		// 8.9.6.4  | 8      | 964
		// 9        | 9      | 0
		// 6.9      | 6      | 9
		// 6.6.6    | 6      | 66
		// 13.6.6.6 | 13     | 666
		case NPPM_GETNPPVERSION:
		{
			const TCHAR* verStr = VERSION_VALUE;
			TCHAR mainVerStr[16];
			TCHAR auxVerStr[16];
			bool isDot = false;
			int j = 0;
			int k = 0;
			for (int i = 0; verStr[i]; ++i)
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

			// if auxVerStr length should less or equal to 3.
			// if auxVer is less 3 digits, the padding (0) will be added.
			bool addZeroPadding = wParam == TRUE;
			if (addZeroPadding)
			{
				size_t nbDigit = lstrlen(auxVerStr);
				if (nbDigit > 0 && nbDigit <= 3)
				{
					if (nbDigit == 3)
					{
						// OK, nothing to do.
					}
					else if (nbDigit == 2)
					{
						auxVerStr[2] = '0';
						auxVerStr[3] = '\0';
					}
					else // if (nbDigit == 1)
					{
						auxVerStr[1] = '0';
						auxVerStr[2] = '0';
						auxVerStr[3] = '\0';
					}
				}
			}

			int mainVer = 0, auxVer = 0;
			if (mainVerStr[0])
				mainVer = generic_atoi(mainVerStr);

			if (auxVerStr[0])
				auxVer = generic_atoi(auxVerStr);

			return MAKELONG(auxVer, mainVer);
		}

		case NPPM_GETCURRENTMACROSTATUS:
		{
			if (_recordingMacro)
				return static_cast<LRESULT>(MacroStatus::RecordInProgress);
			if (_playingBackMacro)
				return static_cast<LRESULT>(MacroStatus::PlayingBack);
			return (_macro.empty()) ? static_cast<LRESULT>(MacroStatus::Idle) : static_cast<LRESULT>(MacroStatus::RecordingStopped);
		}

		case NPPM_GETCURRENTCMDLINE:
		{
			generic_string cmdLineString = nppParam.getCmdLineString();

			if (lParam != 0)
			{
				if (cmdLineString.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<TCHAR*>(lParam), cmdLineString.c_str());
			}
			return cmdLineString.length();
		}

		case NPPM_CREATELEXER:
		{
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			const char* lexer_name = wmc.wchar2char(reinterpret_cast<TCHAR*>(lParam), CP_ACP);
			return (LRESULT) CreateLexer(lexer_name);
		}

		case WM_FRSAVE_INT:
		{
			_macro.push_back(recordedMacroStep(static_cast<int32_t>(wParam), 0, lParam, NULL, recordedMacroStep::mtSavedSnR));
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
				intptr_t lastLine = _pEditView->execute(SCI_GETLINECOUNT) - 1;
				intptr_t currLine = _pEditView->getCurrentLineNumber();
				int indexMacro = _runMacroDlg.getMacro2Exec();
				intptr_t deltaLastLine = 0;
				intptr_t deltaCurrLine = 0;

				Macro m = _macro;

				if (indexMacro != -1)
				{
					vector<MacroShortcut> & ms = nppParam.getMacroList();
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
						deltaLastLine = _pEditView->execute(SCI_GETLINECOUNT) - 1 - lastLine;
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
						if ((currLine > lastLine) || (currLine < 0)
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
			return (LRESULT)_scintillaCtrls4Plugins.createSintilla((lParam ? reinterpret_cast<HWND>(lParam) : hwnd));
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
			//return _scintillaCtrls4Plugins.destroyScintilla(reinterpret_cast<HWND>(lParam));

			// Destroying allocated Scintilla makes Notepad++ crash
			// because created Scintilla view's pointer is added into _referees of Buffer object automatically.
			// The deallocated scintilla view in _referees is used in Buffer::nextUntitledNewNumber().

			// So we do nothing here and let Notepad++ destroy allocated Scintilla while it exits
			// and we keep this message for the sake of compability withe the existing plugins.
			return true;
		}

		case NPPM_GETNBUSERLANG:
		{
			if (lParam)
				*(reinterpret_cast<int *>(lParam)) = IDM_LANG_USER;
			return nppParam.getNbUserLang();
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
			if (!wParam || !lParam) // Clean up current session
			{
				delete _pShortcutMapper;
				_pShortcutMapper = nullptr;
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
			const NppGUI & nppGUI = nppParam.getNppGUI();

			if (nppGUI._caretWidth < 4)
			{
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
				_mainEditView.execute(SCI_SETCARETWIDTH, nppGUI._caretWidth);
				_subEditView.execute(SCI_SETCARETWIDTH, nppGUI._caretWidth);
			}
			else if (nppGUI._caretWidth == 4)
			{
				_mainEditView.execute(SCI_SETCARETWIDTH, 1);
				_subEditView.execute(SCI_SETCARETWIDTH, 1);
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
			}
			else // nppGUI._caretWidth == 5
			{
				_mainEditView.execute(SCI_SETCARETWIDTH, 1);
				_subEditView.execute(SCI_SETCARETWIDTH, 1);
				_mainEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK | CARETSTYLE_BLOCK_AFTER);
				_subEditView.execute(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK | CARETSTYLE_BLOCK_AFTER);
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

		case NPPM_INTERNAL_CARETLINEFRAME:
		{
			_mainEditView.execute(SCI_SETCARETLINEFRAME, lParam);
			_subEditView.execute(SCI_SETCARETLINEFRAME, lParam);
			return TRUE;
		}

		case NPPM_SETEDITORBORDEREDGE:
		{
			bool withBorderEdge = (lParam == 1);
			_mainEditView.setBorderEdge(withBorderEdge);
			_subEditView.setBorderEdge(withBorderEdge);
			return TRUE;
		}

		case NPPM_INTERNAL_VIRTUALSPACE:
		{
			const bool virtualSpace = (nppParam.getSVP())._virtualSpace;

			int virtualSpaceOptions = SCVS_RECTANGULARSELECTION;
			if(virtualSpace)
				virtualSpaceOptions |= SCVS_USERACCESSIBLE | SCVS_NOWRAPLINESTART;

			_mainEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions);
			_subEditView.execute(SCI_SETVIRTUALSPACEOPTIONS, virtualSpaceOptions);

			return TRUE;
		}

		case NPPM_INTERNAL_SCROLLBEYONDLASTLINE:
		{
			const bool endAtLastLine = !(nppParam.getSVP())._scrollBeyondLastLine;
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
			const NppGUI & nppGUI = nppParam.getNppGUI();
			_mainEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			_subEditView.execute(SCI_SETMULTIPLESELECTION, nppGUI._enableMultiSelection);
			return TRUE;
		}

		case NPPM_INTERNAL_SETCARETBLINKRATE:
		{
			const NppGUI & nppGUI = nppParam.getNppGUI();
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
			if (nppParam._isTaskListRBUTTONUP_Active)
			{
				nppParam._isTaskListRBUTTONUP_Active = false;
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
					std::vector<MenuItemUnit>& tmp = nppParam.getContextMenuItems();
					bool copyLink = (_pEditView->getSelectedTextCount() == 0) && _pEditView->getIndicatorRange(URL_INDIC);
					scintillaContextmenu.create(hwnd, tmp, _mainMenuHandle, copyLink);
					scintillaContextmenu.display(p);
					return TRUE;
				}
			}

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case WM_NOTIFY:
		{
			NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
			if (nmhdr->code == NM_CUSTOMDRAW && (nmhdr->hwndFrom == _toolBar.getHSelf()))
			{
				NMTBCUSTOMDRAW* nmtbcd = reinterpret_cast<NMTBCUSTOMDRAW*>(lParam);
				if (nmtbcd->nmcd.dwDrawStage == CDDS_PREERASE)
				{
					if (NppDarkMode::isEnabled())
					{
						FillRect(nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, NppDarkMode::getDarkerBackgroundBrush());
						nmtbcd->clrText = NppDarkMode::getTextColor();
						SetTextColor(nmtbcd->nmcd.hdc, NppDarkMode::getTextColor());
						return CDRF_SKIPDEFAULT;
					}
					else
					{
						return CDRF_DODEFAULT;
					}
				}
			}

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
			// This is an workaround to deal with Microsoft issue in ReadDirectoryChanges notification
			// If command prompt is used to write file continuously (e.g. ping -t 8.8.8.8 > ping.log)
			// Then ReadDirectoryChanges does not detect the change.
			// Fortunately, notification is sent if right click or double click happens on that file
			// Let's leverage this as workaround to enhance npp file monitoring functionality.
			// So calling "PathFileExists" is a workaround here.

			Buffer* currBuf = getCurrentBuffer();
			if (currBuf && currBuf->isMonitoringOn())
				::PathFileExists(currBuf->getFullPathName());

			const NppGUI & nppgui = nppParam.getNppGUI();
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
			return MainFileManager.getFileNameFromBuffer(reinterpret_cast<BufferID>(wParam), reinterpret_cast<TCHAR *>(lParam));
		}

		case NPPM_INTERNAL_ENABLECHECKDOCOPT:
		{
			NppGUI& nppgui = nppParam.getNppGUI();
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
			if (wParam != WA_INACTIVE && _pEditView && _pNonEditView)
			{
				_pEditView->getFocus();
				auto x = _pEditView->execute(SCI_GETXOFFSET);
				_pEditView->execute(SCI_SETXOFFSET, x);
				x = _pNonEditView->execute(SCI_GETXOFFSET);
				_pNonEditView->execute(SCI_SETXOFFSET, x);
			}
			return TRUE;
		}

		case WM_SYNCPAINT:
		{
			RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
			break;
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
			addHotSpot(& _mainEditView);

			_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());
			_subEditView.performGlobalStyles();
			addHotSpot(& _subEditView);

			_findReplaceDlg.updateFinderScintilla();

			drawTabbarColoursFromStylerArray();

			drawDocumentMapColoursFromStylerArray();

			// Update default fg/bg colors in Parameters for both internal/plugins docking dialog
			const Style* pStyle = NppParameters::getInstance().getGlobalStylers().findByID(STYLE_DEFAULT);
			if (pStyle)
			{
				NppParameters::getInstance().setCurrentDefaultFgColor(pStyle->_fgColor);
				NppParameters::getInstance().setCurrentDefaultBgColor(pStyle->_bgColor);
				drawAutocompleteColoursFromTheme(pStyle->_fgColor, pStyle->_bgColor);
			}

			AutoCompletion::drawAutocomplete(_pEditView);
			AutoCompletion::drawAutocomplete(_pNonEditView);

			NppDarkMode::calculateTreeViewStyle();
			auto refreshOnlyTreeView = static_cast<LPARAM>(TRUE);

			// Set default fg/bg colors on internal docking dialog
			if (pStyle && _pFuncList)
			{
				_pFuncList->setBackgroundColor(pStyle->_bgColor);
				_pFuncList->setForegroundColor(pStyle->_fgColor);
				::SendMessage(_pFuncList->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, refreshOnlyTreeView);
			}

			if (pStyle && _pAnsiCharPanel)
			{
				_pAnsiCharPanel->setBackgroundColor(pStyle->_bgColor);
				_pAnsiCharPanel->setForegroundColor(pStyle->_fgColor);
			}

			if (pStyle && _pDocumentListPanel)
			{
				_pDocumentListPanel->setBackgroundColor(pStyle->_bgColor);
				_pDocumentListPanel->setForegroundColor(pStyle->_fgColor);
			}

			if (pStyle && _pClipboardHistoryPanel)
			{
				_pClipboardHistoryPanel->setBackgroundColor(pStyle->_bgColor);
				_pClipboardHistoryPanel->setForegroundColor(pStyle->_fgColor);
				_pClipboardHistoryPanel->redraw(true);
			}

			if (pStyle && _pProjectPanel_1)
			{
				_pProjectPanel_1->setBackgroundColor(pStyle->_bgColor);
				_pProjectPanel_1->setForegroundColor(pStyle->_fgColor);
				::SendMessage(_pProjectPanel_1->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, refreshOnlyTreeView);
			}

			if (pStyle && _pProjectPanel_2)
			{
				_pProjectPanel_2->setBackgroundColor(pStyle->_bgColor);
				_pProjectPanel_2->setForegroundColor(pStyle->_fgColor);
				::SendMessage(_pProjectPanel_2->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, refreshOnlyTreeView);
			}

			if (pStyle && _pProjectPanel_3)
			{
				_pProjectPanel_3->setBackgroundColor(pStyle->_bgColor);
				_pProjectPanel_3->setForegroundColor(pStyle->_fgColor);
				::SendMessage(_pProjectPanel_3->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, refreshOnlyTreeView);
			}

			if (pStyle && _pFileBrowser)
			{
				_pFileBrowser->setBackgroundColor(pStyle->_bgColor);
				_pFileBrowser->setForegroundColor(pStyle->_fgColor);
				::SendMessage(_pFileBrowser->getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, refreshOnlyTreeView);
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

		case WM_UPDATEMAINMENUBITMAPS:
		{
			setupColorSampleBitmapsOnMainMenuItems();
			return TRUE;
		}

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
		{
			if (message == WM_QUERYENDSESSION)
			{
				nppParam.queryEndSessionStart();
			}

			if (nppParam.isQueryEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
			{
				generic_string issueFn = nppLogNulContentCorruptionIssue;
				issueFn += TEXT(".log");
				generic_string nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);

				writeLog(nppIssueLog.c_str(), "WM_QUERYENDSESSION =====================================");
			}

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

				const NppGUI & nppgui = nppParam.getNppGUI();

				bool isSnapshotMode = nppgui.isSnapshotMode();

				if (isSnapshotMode)
				{
					::LockWindowUpdate(hwnd);
					MainFileManager.backupCurrentBuffer();
				}

				Session currentSession;
				if (!((nppgui._multiInstSetting == monoInst) && !nppgui._rememberLastSession))
					getCurrentOpenedFiles(currentSession, true);

				if (nppgui._rememberLastSession)
				{
					//Lock the recent file list so it isnt populated with opened files
					//Causing them to show on restart even though they are loaded by session
					_lastRecentFileList.setLock(true);	//only lock when the session is remembered
				}
				_isAttemptingCloseOnQuit = true;
				bool allClosed = fileCloseAll(false, isSnapshotMode);	//try closing files before doing anything else
				_isAttemptingCloseOnQuit = false;

				if (nppgui._rememberLastSession)
					_lastRecentFileList.setLock(false);	//only lock when the session is remembered

				if (!saveProjectPanelsParams()) allClosed = false; //writeProjectPanelsSettings
				saveFileBrowserParam();

				if (!allClosed)
				{
					//User cancelled the shutdown
					scnN.nmhdr.code = NPPN_CANCELSHUTDOWN;
					_pluginsManager.notify(&scnN);
					
					if (isSnapshotMode)
						::LockWindowUpdate(NULL);
					return FALSE;
				}

				if (_beforeSpecialView._isFullScreen)	//closing, return to windowed mode
					fullScreenToggle();
				if (_beforeSpecialView._isPostIt)		//closing, return to windowed mode
					postItToggle();

				if (_configStyleDlg.isCreated() && ::IsWindowVisible(_configStyleDlg.getHSelf()))
					_configStyleDlg.restoreGlobalOverrideValues();

				scnN.nmhdr.code = NPPN_SHUTDOWN;
				_pluginsManager.notify(&scnN);


				saveScintillasZoom(); 
				saveGUIParams(); //writeGUIParams writeScintillaParams
				saveFindHistory(); //writeFindHistory
				_lastRecentFileList.saveLRFL(); //writeRecentFileHistorySettings, writeHistory
				//
				// saving config.xml
				//
				nppParam.saveConfig_xml();

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

				//
				// saving session.xml into loaded session if a saved session is loaded and saveLoadedSessionOnExit option is enabled
				//
				generic_string loadedSessionFilePath = nppParam.getLoadedSessionFilePath();
				if (!loadedSessionFilePath.empty() && PathFileExists(loadedSessionFilePath.c_str()))
					nppParam.writeSession(currentSession, loadedSessionFilePath.c_str());

				// write settings on cloud if enabled, if the settings files don't exist
				if (nppgui._cloudPath != TEXT("") && nppParam.isCloudPathChanged())
				{
					bool isOK = nppParam.writeSettingsFilesOnCloudForThe1stTime(nppgui._cloudPath);
					if (!isOK)
					{
						_nativeLangSpeaker.messageBox("SettingsOnCloudError",
							hwnd,
							TEXT("It seems the path of settings on cloud is set on a read only drive,\ror on a folder needed privilege right for writing access.\rYour settings on cloud will be canceled. Please reset a coherent value via Preference dialog."),
							TEXT("Settings on Cloud"),
							MB_OK | MB_APPLMODAL);
						nppParam.removeCloudChoice();
					}
				}

				if (isSnapshotMode)
					::LockWindowUpdate(NULL);

				//Sends WM_DESTROY, Notepad++ will end
				if (message == WM_CLOSE)
					::DestroyWindow(hwnd);

				generic_string updaterFullPath = nppParam.getWingupFullPath();
				if (!updaterFullPath.empty())
				{
					Process updater(updaterFullPath.c_str(), nppParam.getWingupParams().c_str(), nppParam.getWingupDir().c_str());
					updater.run(nppParam.shouldDoUAC());
				}
			}

			// _isEndingSessionButNotReady is true means WM_QUERYENDSESSION is sent but no time to finish saving data
            // then WM_ENDSESSION is sent with wParam == FALSE - Notepad++ should exit in this case
			if (_isEndingSessionButNotReady) 
				::DestroyWindow(hwnd);

			if (message == WM_CLOSE)
				return 0;

			return TRUE;
		}

		case WM_ENDSESSION:
		{
			if (nppParam.isQueryEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
			{
				generic_string issueFn = nppLogNulContentCorruptionIssue;
				issueFn += TEXT(".log");
				generic_string nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);

				writeLog(nppIssueLog.c_str(), "WM_ENDSESSION");
			}

			if (wParam == TRUE)
			{
				::DestroyWindow(hwnd);
			}
			else
			{
				_isEndingSessionButNotReady = true;
			}
			return 0;
		}

		case WM_DESTROY:
		{
			if (nppParam.isQueryEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
			{
				generic_string issueFn = nppLogNulContentCorruptionIssue;
				issueFn += TEXT(".log");
				generic_string nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);

				writeLog(nppIssueLog.c_str(), "WM_DESTROY");
			}

			killAllChildren();
			::PostQuitMessage(0);
			_pPublicInterface->gNppHWND = NULL;
			return TRUE;
		}

		case WM_SYSCOMMAND:
		{
			const NppGUI & nppgui = (nppParam.getNppGUI());
			if ((nppgui._isMinimizedToTray || _pPublicInterface->isPrelaunch()) && (wParam == SC_MINIMIZE))
			{
				if (nullptr == _pTrayIco)
					_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, NPPM_INTERNAL_MINIMIZED_TRAY, ::LoadIcon(_pPublicInterface->getHinst(), MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

				_pTrayIco->doTrayIcon(ADD);
				_dockingManager.showFloatingContainers(false);
				minimizeDialogs();
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

		case NPPM_INTERNAL_MINIMIZED_TRAY:
		{
			switch (lParam)
			{
				//case WM_LBUTTONDBLCLK:
				case WM_LBUTTONUP :
				{
					_pEditView->getFocus();
					::ShowWindow(hwnd, SW_SHOW);
					_dockingManager.showFloatingContainers(true);
					restoreMinimizeDialogs();

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

		case NPPM_ADDTOOLBARICON_DEPRECATED:
		{
			_toolBar.registerDynBtn(static_cast<UINT>(wParam), reinterpret_cast<toolbarIcons*>(lParam), _pPublicInterface->getAbsentIcoHandle());
			return TRUE;
		}

		case NPPM_ADDTOOLBARICON_FORDARKMODE:
		{
			_toolBar.registerDynBtnDM(static_cast<UINT>(wParam), reinterpret_cast<toolbarIconsWithDarkMode*>(lParam));
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
			return (NppParameters::getInstance()).getWinVersion();
		}

		case NPPM_MAKECURRENTBUFFERDIRTY:
		{
			_pEditView->getCurrentBuffer()->setDirty(true);
			return TRUE;
		}

		case NPPM_GETENABLETHEMETEXTUREFUNC:
		{
			return (LRESULT)nppParam.getEnableThemeDlgTexture();
		}

		case NPPM_GETPLUGINSCONFIGDIR:
		{
			generic_string userPluginConfDir = nppParam.getUserPluginConfDir();
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
			generic_string pluginHomePath = nppParam.getPluginRootDir();
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

		case NPPM_GETSETTINGSONCLOUDPATH:
		{
			const NppGUI & nppGUI = nppParam.getNppGUI();
			generic_string settingsOnCloudPath = nppGUI._cloudPath;
			if (lParam != 0)
			{
				if (settingsOnCloudPath.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<TCHAR *>(lParam), settingsOnCloudPath.c_str());
			}
			return settingsOnCloudPath.length();
		}

		case NPPM_SETLINENUMBERWIDTHMODE:
		{
			if (lParam != LINENUMWIDTH_DYNAMIC && lParam != LINENUMWIDTH_CONSTANT)
				return FALSE;

			ScintillaViewParams &svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());
			svp._lineNumberMarginDynamicWidth = lParam == LINENUMWIDTH_DYNAMIC;
			::SendMessage(hwnd, WM_COMMAND, IDM_VIEW_LINENUMBER, 0);

			return TRUE;
		}

		case NPPM_GETLINENUMBERWIDTHMODE:
		{
			const ScintillaViewParams &svp = nppParam.getSVP();
			return svp._lineNumberMarginDynamicWidth ? LINENUMWIDTH_DYNAMIC : LINENUMWIDTH_CONSTANT;
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

			NppGUI & nppGUI = (NppParameters::getInstance()).getNppGUI();
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

			NppGUI & nppGUI = nppParam.getNppGUI();
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
			NppGUI & nppGUI = nppParam.getNppGUI();
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
			const NppGUI & nppGUI = nppParam.getNppGUI();
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

		case NPPM_INTERNAL_CRLFFORMCHANGED:
		{
			_mainEditView.setCRLF();
			_subEditView.setCRLF();
			return TRUE;
		}

		case NPPM_INTERNAL_CRLFLAUNCHSTYLECONF:
		{
			// Launch _configStyleDlg (create or display it)
			command(IDM_LANGSTYLE_CONFIG_DLG);

			// go into the section we need
			_configStyleDlg.goToSection(TEXT("Global Styles:EOL custom color"));

			return TRUE;
		}

		case NPPM_INTERNAL_LAUNCHPREFERENCES:
		{
			// Launch _configStyleDlg (create or display it)
			command(IDM_SETTING_PREFERENCE);

			// go into the section we need
			_preference.goToSection(wParam, lParam);

			return TRUE;
		}

		case NPPM_INTERNAL_DISABLEAUTOUPDATE:
		{
			//printStr(TEXT("you've got me"));
			NppGUI & nppGUI = nppParam.getNppGUI();
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

		case NPPM_GETEXTERNALLEXERAUTOINDENTMODE:
		{
			int index = nppParam.getExternalLangIndexFromName(reinterpret_cast<TCHAR*>(wParam));
			if (index < 0)
				return FALSE;

			*(reinterpret_cast<ExternalLexerAutoIndentMode*>(lParam)) = nppParam.getELCFromIndex(index)._autoIndentMode;
			return TRUE;
		}

		case NPPM_SETEXTERNALLEXERAUTOINDENTMODE:
		{
			int index = nppParam.getExternalLangIndexFromName(reinterpret_cast<TCHAR*>(wParam));
			if (index < 0)
				return FALSE;

			nppParam.getELCFromIndex(index)._autoIndentMode = static_cast<ExternalLexerAutoIndentMode>(lParam);
			return TRUE;
		}

		case NPPM_ISAUTOINDENTON:
		{
			return nppParam.getNppGUI()._maitainIndent;
		}

		case NPPM_ISDARKMODEENABLED:
		{
			return NppDarkMode::isEnabled();
		}

		case NPPM_GETDARKMODECOLORS:
		{
			if (static_cast<size_t>(wParam) != sizeof(NppDarkMode::Colors))
				return static_cast<LRESULT>(false);

			NppDarkMode::Colors* currentColors = reinterpret_cast<NppDarkMode::Colors*>(lParam);

			if (currentColors != NULL)
			{
				currentColors->background = NppDarkMode::getBackgroundColor();
				currentColors->softerBackground = NppDarkMode::getSofterBackgroundColor();
				currentColors->hotBackground = NppDarkMode::getHotBackgroundColor();
				currentColors->pureBackground = NppDarkMode::getDarkerBackgroundColor();
				currentColors->errorBackground = NppDarkMode::getErrorBackgroundColor();
				currentColors->text = NppDarkMode::getTextColor();
				currentColors->darkerText = NppDarkMode::getDarkerTextColor();
				currentColors->disabledText = NppDarkMode::getDisabledTextColor();
				currentColors->linkText = NppDarkMode::getLinkTextColor();
				currentColors->edge = NppDarkMode::getEdgeColor();
				currentColors->hotEdge = NppDarkMode::getHotEdgeColor();
				currentColors->disabledEdge = NppDarkMode::getDisabledEdgeColor();

				return static_cast<LRESULT>(true);
			}

			return static_cast<LRESULT>(false);
		}

		case NPPM_DOCLISTDISABLEPATHCOLUMN:
		case NPPM_DOCLISTDISABLEEXTCOLUMN:
		{
			BOOL isOff = static_cast<BOOL>(lParam);
			NppGUI & nppGUI = nppParam.getNppGUI();

			if (message == NPPM_DOCLISTDISABLEEXTCOLUMN)
				nppGUI._fileSwitcherWithoutExtColumn = isOff == TRUE;
			else
				nppGUI._fileSwitcherWithoutPathColumn = isOff == TRUE;

			if (_pDocumentListPanel)
			{
				_pDocumentListPanel->reload();
			}
			// else nothing to do
			return TRUE;
		}

		case NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR:
		case NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR:
		{
			return (message == NPPM_GETEDITORDEFAULTFOREGROUNDCOLOR
					?(NppParameters::getInstance()).getCurrentDefaultFgColor()
					:(NppParameters::getInstance()).getCurrentDefaultBgColor());
		}

		case NPPM_SHOWDOCLIST:
		{
			BOOL toShow = static_cast<BOOL>(lParam);
			if (toShow)
			{
				if (!_pDocumentListPanel || !_pDocumentListPanel->isVisible())
					launchDocumentListPanel();
			}
			else
			{
				if (_pDocumentListPanel)
					_pDocumentListPanel->display(false);
			}
			return TRUE;
		}

		case NPPM_ISDOCLISTSHOWN:
		{
			if (!_pDocumentListPanel)
				return FALSE;
			return _pDocumentListPanel->isVisible();
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
			const TCHAR *appDataNpp = nppParam.getAppDataNppDir();
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
			_lastRecentFileList.setUserMaxNbLRF(nppParam.getNbMaxRecentFile());
			break;
		}

		case NPPM_INTERNAL_EDGEMULTISETSIZE:
		{
			_mainEditView.execute(SCI_MULTIEDGECLEARALL);
			_subEditView.execute(SCI_MULTIEDGECLEARALL);

			ScintillaViewParams &svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());

			COLORREF multiEdgeColor = liteGrey;
			const Style * pStyle = NppParameters::getInstance().getMiscStylerArray().findByName(TEXT("Edge colour"));
			if (pStyle)
			{
				multiEdgeColor = pStyle->_fgColor;
			}

			const size_t twoPower13 = 8192;
			size_t nbColAdded = 0;
			for (auto i : svp._edgeMultiColumnPos)
			{
				// it's absurd to set columns beyon 8000, even it's a long line.
				// So let's ignore all the number greater than 2^13
				if (i > twoPower13)
					continue;

				_mainEditView.execute(SCI_MULTIEDGEADDLINE, i, multiEdgeColor);
				_subEditView.execute(SCI_MULTIEDGEADDLINE, i, multiEdgeColor);

				++nbColAdded;
			}

			int mode;
			switch (nbColAdded)
			{
				case 0:
				{
					mode = EDGE_NONE;
					break;
				}
				case 1:
				{
					if (svp._isEdgeBgMode)
					{
						mode = EDGE_BACKGROUND;
						_mainEditView.execute(SCI_SETEDGECOLUMN, svp._edgeMultiColumnPos[0]);
						_subEditView.execute(SCI_SETEDGECOLUMN, svp._edgeMultiColumnPos[0]);
					}
					else
					{
						mode = EDGE_MULTILINE;
					}
					break;
				}
				default:
					mode = EDGE_MULTILINE;
			}

			_mainEditView.execute(SCI_SETEDGEMODE, mode);
			_subEditView.execute(SCI_SETEDGEMODE, mode);
		}
		break;

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
			const NppGUI & nppgui = nppParam.getNppGUI();
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(hwnd, _mainMenuHandle);

			return TRUE;
		}

		case WM_EXITMENULOOP:
		{
			const NppGUI & nppgui = nppParam.getNppGUI();
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(hwnd, NULL);
			_sysMenuEntering = false;
			return FALSE;
		}

		case WM_DPICHANGED:
		{
			return TRUE;
		}

		case NPPM_INTERNAL_UPDATECLICKABLELINKS:
		{
			ScintillaEditView* pView = reinterpret_cast<ScintillaEditView*>(wParam);
			if (pView == NULL)
			{
				addHotSpot(_pEditView);
				addHotSpot(_pNonEditView);
			}
			else
			{
				addHotSpot(pView);
			}
		}

		case NPPM_INTERNAL_UPDATETEXTZONEPADDING:
		{
			ScintillaViewParams &svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());
			if (_beforeSpecialView._isDistractionFree)
			{
				int paddingLen = svp.getDistractionFreePadding(_pEditView->getWidth());
				_pEditView->execute(SCI_SETMARGINLEFT, 0, paddingLen);
				_pEditView->execute(SCI_SETMARGINRIGHT, 0, paddingLen);
			}
			else
			{
				_mainEditView.execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
				_mainEditView.execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);
				_subEditView.execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
				_subEditView.execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_REFRESHWORKDIR:
		{
			const Buffer* buf = _pEditView->getCurrentBuffer();
			generic_string path = buf ? buf->getFullPathName() : _T("");
			PathRemoveFileSpec(path);
			setWorkingDir(path.c_str());
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
							fileSave(_pDocTab->getBufferByIndex(nmdlg->Items[i]));
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

						::SendMessage(_pDocTab->getHParent(), NPPM_INTERNAL_DOCORDERCHANGED, 0, _pDocTab->getCurrentTabIndex());

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
