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
#include <uxtheme.h> // for EnableThemeDialogTexture
#include <format>
#include <windowsx.h> // for GET_X_LPARAM, GET_Y_LPARAM
#include <atomic>
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

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

std::atomic<bool> g_bNppExitFlag{ false };


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

// app-restart feature needs Win10 20H1+ (builds 18963+), but it was quietly introduced earlier in the Fall Creators Update (b1709+)
bool SetOSAppRestart()
{
	NppParameters& nppParam = NppParameters::getInstance();
	if (nppParam.getWinVersion() < WV_WIN10)
		return false; // unsupported

	bool bRet = false;
	bool bUnregister = nppParam.isRegForOSAppRestartDisabled();

	wstring nppIssueLog;
	if (nppParam.doNppLogNulContentCorruptionIssue())
	{
		wstring issueFn = nppLogNulContentCorruptionIssue;
		issueFn += L".log";
		nppIssueLog = nppParam.getUserPath();
		pathAppend(nppIssueLog, issueFn);
	}

	wchar_t wszCmdLine[RESTART_MAX_CMD_LINE] = { 0 };
	DWORD cchCmdLine = _countof(wszCmdLine);
	DWORD dwPreviousFlags = 0;
	HRESULT hr = ::GetApplicationRestartSettings(::GetCurrentProcess(), wszCmdLine, &cchCmdLine, &dwPreviousFlags);
	if (bUnregister)
	{
		// unregistering (disabling) request

		if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
		{
			// has not been registered before, nothing to do here
			bRet = true;
		}
		else
		{
			if (hr == S_OK)
			{
				// has been already registered before, try to unregister
				if (::UnregisterApplicationRestart() == S_OK)
				{
					bRet = true;
				}
				else
				{
					if (nppParam.doNppLogNulContentCorruptionIssue())
					{
						std::string msg = "ERROR: UnregisterApplicationRestart WINAPI failed! (HRESULT: ";
						msg += std::format("{:#010x}", hr);
						msg += ")";
						writeLog(nppIssueLog.c_str(), msg.c_str());
					}
				}
			}
			else
			{
				if (nppParam.doNppLogNulContentCorruptionIssue())
				{
					std::string msg = "ERROR: GetApplicationRestartSettings WINAPI failed! (HRESULT: ";
					msg += std::format("{:#010x}", hr);
					msg += ")";
					writeLog(nppIssueLog.c_str(), msg.c_str());
				}
			}
		}
	}
	else
	{
		// registering request

		if (hr == S_OK)
			::UnregisterApplicationRestart(); // remove a previous registration 1st

		if (nppParam.getCmdLineString().length() >= RESTART_MAX_CMD_LINE)
		{
			if (nppParam.doNppLogNulContentCorruptionIssue())
			{
				std::string msg = "WARNING: Skipping the RegisterApplicationRestart WINAPI call because of the cmdline length exceeds the RESTART_MAX_CMD_LINE! \n(current cmdline length: ";
				msg += std::to_string(nppParam.getCmdLineString().length());
				msg += " chars)";
				writeLog(nppIssueLog.c_str(), msg.c_str());
			}
		}
		else
		{
			// do not restart the process:
			// RESTART_NO_CRASH  (1) ... for termination due to application crashes
			// RESTART_NO_HANG   (2) ... for termination due to application hangs
			// RESTART_NO_PATCH  (4) ... for termination due to patch installations
			// RESTART_NO_REBOOT (8) ... when the system is rebooted
			hr = ::RegisterApplicationRestart(nppParam.getCmdLineString().c_str(), RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_PATCH);
			if (hr == S_OK)
			{
				bRet = true;
			}
			else
			{
				if (nppParam.doNppLogNulContentCorruptionIssue())
				{
					std::string msg = "ERROR: RegisterApplicationRestart WINAPI failed! (HRESULT: ";
					msg += std::format("{:#010x}", hr);
					msg += ")";
					writeLog(nppIssueLog.c_str(), msg.c_str());
				}
			}
		}
	}

	return bRet;
}

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

				SetOSAppRestart();

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
			return _notepad_plus_plus_core.process(hwnd, message, wParam, lParam);
		}
	}
}

// Used by NPPM_GETFILENAMEATCURSOR
int CharacterIs(wchar_t c, const wchar_t *any)
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

			const bool enableDarkMode = NppDarkMode::isExperimentalActive();
			NppGUI& nppGUI = nppParam.getNppGUI();

			// Windows mode is enabled
			// and don't change if Notepad++ is already in same mode as OS after changing OS mode
			if (NppDarkMode::isWindowsModeEnabled() && (enableDarkMode != NppDarkMode::isEnabled()))
			{
				nppGUI._darkmode._isEnabled = enableDarkMode;
				if (!_preference.isCreated())
				{
					const int iconState = NppDarkMode::getToolBarIconSet(NppDarkMode::isEnabled());
					toolBarStatusType state = (iconState == -1) ? _toolBar.getState() : static_cast<toolBarStatusType>(iconState);
					switch (state)
					{
						case TB_SMALL:
							_toolBar.reduce();
							break;

						case TB_LARGE:
							_toolBar.enlarge();
							break;

						case TB_SMALL2:
							_toolBar.reduceToSet2();
							break;

						case TB_LARGE2:
							_toolBar.enlargeToSet2();
							break;

						case TB_STANDARD:
							_toolBar.setToBmpIcons();
							break;
					}
					NppDarkMode::refreshDarkMode(hwnd, false);
				}
				else
				{
					HWND hSubDlg = _preference._darkModeSubDlg.getHSelf();

					// don't use IDC_RADIO_DARKMODE_FOLLOWWINDOWS, it is only for button,
					// it calls NppDarkMode::handleSettingChange, which is not needed here
					::SendMessage(hSubDlg, WM_COMMAND, IDC_RADIO_DARKMODE_DARKMODE, 0);
				}
			}

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			refreshDarkMode(static_cast<bool>(wParam));
			// Notify plugins that Dark Mode changed
			SCNotification scnN{};
			scnN.nmhdr.code = NPPN_DARKMODECHANGED;
			scnN.nmhdr.hwndFrom = hwnd;
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
			wchar_t *userLangName = reinterpret_cast<wchar_t *>(lParam);
			if (!userLangName || !userLangName[0])
				return FALSE;

			wstring name{userLangName};

			//loop through buffers and reset the language (L_USER, L"")) if (L_USER, name)
			for (size_t i = 0; i < MainFileManager.getNbBuffers(); ++i)
			{
				Buffer* buf = MainFileManager.getBufferByIndex(i);
				if (buf->getLangType() == L_USER && name == buf->getUserDefineLangName())
					buf->setLangType(L_USER, L"");
			}
			return TRUE;
		}

		case WM_RENAME_USERLANG:
		{
			if (!lParam || !((reinterpret_cast<wchar_t *>(lParam))[0]) || !wParam || !((reinterpret_cast<wchar_t *>(wParam))[0]))
				return FALSE;

			wstring oldName{ reinterpret_cast<wchar_t *>(lParam) };
			wstring newName{ reinterpret_cast<wchar_t *>(wParam) };

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
			constexpr int strSize = FINDREPLACE_MAXLENGTH;

			bool isFirstTime = !_findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _nativeLangSpeaker.isRTL());

			const NppGUI& nppGui = nppParam.getNppGUI();
			if (nppGui._fillFindFieldWithSelected)
			{
				wchar_t str[strSize]{};
				_pEditView->getGenericSelectedText(str, strSize, nppGui._fillFindFieldSelectCaret);
				_findReplaceDlg.setSearchText(str);
			}

			if (isFirstTime)
				_nativeLangSpeaker.changeFindReplaceDlgLang(_findReplaceDlg);
			_findReplaceDlg.launchFindInFilesDlg();
			setFindReplaceFolderFilter(reinterpret_cast<const wchar_t*>(wParam), reinterpret_cast<const wchar_t*>(lParam));

			return TRUE;
		}

		case NPPM_INTERNAL_FINDINPROJECTS:
		{
			constexpr int strSize = FINDREPLACE_MAXLENGTH;
			wchar_t str[strSize]{};

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
			constexpr int strSize = FINDREPLACE_MAXLENGTH;
			wchar_t str[strSize]{};
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
			BufferID id = doOpen(reinterpret_cast<const wchar_t *>(lParam));
			if (id != BUFFER_INVALID)
				return switchToFile(id);
			break;
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
			wchar_t longNameFullpath[MAX_PATH]{};
			const wchar_t* pFilePath = reinterpret_cast<const wchar_t*>(lParam);
			wcscpy_s(longNameFullpath, MAX_PATH, pFilePath);
			if (wcschr(longNameFullpath, '~'))
			{
				::GetLongPathName(longNameFullpath, longNameFullpath, MAX_PATH);
			}

			BufferID id = MainFileManager.getBufferFromName(longNameFullpath);
			if (id != BUFFER_INVALID)
				result = doReload(id, wParam != 0);
			
			break; // For relaying this message to other plugin by calling "_pluginsManager.relayNppMessages(message, wParam, lParam)" at the end.
		}

		case NPPM_SWITCHTOFILE :
		{
			BufferID id = MainFileManager.getBufferFromName(reinterpret_cast<const wchar_t *>(lParam));
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
			const wchar_t *filename = reinterpret_cast<const wchar_t *>(lParam);
			if (!filename) return FALSE;
			return doSave(currentBufferID, filename, asCopy);
		}

		case NPPM_SAVEALLFILES:
		{
			return fileSaveAll();
		}

		case NPPM_SAVEFILE:
		{
			return fileSaveSpecific(reinterpret_cast<const wchar_t *>(lParam));
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
			SCNotification scnN{};
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
						wstring pluginMessage { nppParam.getCmdLineParams()._pluginMessage };
						if (!pluginMessage.empty())
						{
							SCNotification scnN{};
							scnN.nmhdr.code = NPPN_CMDLINEPLUGINMSG;
							scnN.nmhdr.hwndFrom = hwnd;
							scnN.nmhdr.idFrom = reinterpret_cast<uptr_t>(pluginMessage.c_str());
							_pluginsManager.notify(&scnN);
						}
					}
					else
					{
#ifdef DEBUG 
						printStr(L"sizeof(CmdLineParams) != cmdLineParamsSize\rCmdLineParams is formed by an instance of another version,\rwhereas your CmdLineParams has been modified in this instance.");
#endif
					}

					NppGUI nppGui = (NppGUI)nppParam.getNppGUI();
					nppGui._isCmdlineNosessionActivated = cmdLineParam->_isNoSession;
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

		case NPPM_INTERNAL_CHANGETABBARICONSET:
		{
			_mainDocTab.changeIconSet(static_cast<unsigned char>(lParam));
			_subDocTab.changeIconSet(static_cast<unsigned char>(lParam));

			changeDocumentListIconSet(static_cast<bool>(wParam));
			return TRUE;
		}

		case NPPM_INTERNAL_RELOADNATIVELANG:
		{
			reloadLang();

			bool doNotif = wParam;
			if (doNotif)
			{
				SCNotification scnN{};
				scnN.nmhdr.code = NPPN_NATIVELANGCHANGED;
				scnN.nmhdr.hwndFrom = _pPublicInterface->getHSelf();
				scnN.nmhdr.idFrom = 0;
				_pluginsManager.notify(&scnN);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_RELOADSTYLERS:
		{
			loadStyles();
			return TRUE;
		}

		case NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED:
		{
			SCNotification scnN{};
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
			wchar_t str[MAX_PATH] = { '\0' };
			// par defaut : NPPM_GETCURRENTDIRECTORY
			wcscpy_s(str, _pEditView->getCurrentBuffer()->getFullPathName());
			wchar_t* fileStr = str;

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

			// For the compability reason, if wParam is 0, then we assume the size of wstring buffer (lParam) is large enough.
			// otherwise we check if the wstring buffer size is enough for the wstring to copy.
			if (wParam != 0)
			{
				if (lstrlen(fileStr) >= int(wParam))
				{
					return FALSE;
				}
			}

			lstrcpy(reinterpret_cast<wchar_t *>(lParam), fileStr);
			return TRUE;
		}

		case NPPM_GETCURRENTWORD:
		case NPPM_GETCURRENTLINESTR:
		{
			const int strSize = CURRENTWORD_MAXLENGTH;
			wchar_t str[strSize] = { '\0' };
			wchar_t *pTchar = reinterpret_cast<wchar_t *>(lParam);

			if (message == NPPM_GETCURRENTWORD)
				_pEditView->getGenericSelectedText(str, strSize);
			else if (message == NPPM_GETCURRENTLINESTR)
				_pEditView->getLine(_pEditView->getCurrentLineNumber(), str, strSize);

			// For the compability reason, if wParam is 0, then we assume the size of wstring buffer (lParam) is large enough.
			// otherwise we check if the wstring buffer size is enough for the wstring to copy.
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

		case NPPM_GETFILENAMEATCURSOR: // wParam = buffer length, lParam = (wchar_t*)buffer
		{
			constexpr int strSize = CURRENTWORD_MAXLENGTH;
			wchar_t str[strSize]{};
			int hasSlash = 0;

			_pEditView->getGenericSelectedText(str, strSize); // this is either the selected text, or the word under the cursor if there is no selection
			hasSlash = FALSE;
			for (int i = 0; str[i] != 0; i++)
				if (CharacterIs(str[i], L"\\/"))
					hasSlash = TRUE;

			if (hasSlash == FALSE)
			{
				// it's not a full file name so try to find the beginning and ending of it
				intptr_t start = 0;
				intptr_t end = 0;
				const wchar_t *delimiters;
				wchar_t strLine[strSize]{};
				size_t lineNumber = 0;
				intptr_t col = 0;

				lineNumber = _pEditView->getCurrentLineNumber();
				col = _pEditView->execute(SCI_GETCURRENTPOS) - _pEditView->execute(SCI_POSITIONFROMLINE, lineNumber);
				_pEditView->getLine(lineNumber, strLine, strSize);

				// find the start
				start = col;
				delimiters = L" \t[(\"<>";
				while ((start > 0) && (CharacterIs(strLine[start], delimiters) == FALSE))
					start--;

				if (CharacterIs(strLine[start], delimiters)) start++;

				// find the end
				end = col;
				delimiters = L" \t:()[]<>\"\r\n";
				while ((strLine[end] != 0) && (CharacterIs(strLine[end], delimiters) == FALSE)) end++;

				lstrcpyn(str, &strLine[start], static_cast<int>(end - start + 1));
			}

			if (lstrlen(str) >= int(wParam))	//buffer too small
			{
				return FALSE;
			}
			else //buffer large enough, perform safe copy
			{
				wchar_t* pTchar = reinterpret_cast<wchar_t*>(lParam);
				lstrcpyn(pTchar, str, static_cast<int32_t>(wParam));
				return TRUE;
			}
		}

		case NPPM_GETNPPFULLFILEPATH:
		case NPPM_GETNPPDIRECTORY:
		{
			constexpr int strSize = MAX_PATH;
			wchar_t str[strSize]{};

			::GetModuleFileName(NULL, str, strSize);

			if (message == NPPM_GETNPPDIRECTORY)
				PathRemoveFileSpec(str);

			// For the compability reason, if wParam is 0, then we assume the size of wstring buffer (lParam) is large enough.
			// otherwise we check if the wstring buffer size is enough for the wstring to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))
				{
					return FALSE;
				}
			}

			lstrcpy(reinterpret_cast<wchar_t *>(lParam), str);
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
				*id = -1; // cannot happen
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
			else
				return 0;
		}

		case NPPM_GETOPENFILENAMESPRIMARY:
		case NPPM_GETOPENFILENAMESSECOND:
		case NPPM_GETOPENFILENAMES:
		{
			if (!wParam)
				return 0;

			wchar_t** fileNames = reinterpret_cast<wchar_t**>(wParam);
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
			size_t nbSessionFiles = 0;
			const wchar_t* sessionFileName = reinterpret_cast<const wchar_t*>(lParam);
			BOOL* pbIsValidXML = reinterpret_cast<BOOL*>(wParam);
			if (pbIsValidXML)
				*pbIsValidXML = false;
			if (sessionFileName && (sessionFileName[0] != '\0'))
			{
				Session session2Load;
				if (nppParam.loadSession(session2Load, sessionFileName, true))
				{
					if (pbIsValidXML)
						*pbIsValidXML = true;
					nbSessionFiles = session2Load.nbMainFiles() + session2Load.nbSubFiles();
				}
			}
			return nbSessionFiles;
		}

		case NPPM_GETSESSIONFILES:
		{
			const wchar_t *sessionFileName = reinterpret_cast<const wchar_t *>(lParam);
			wchar_t **sessionFileArray = reinterpret_cast<wchar_t **>(wParam);

			if ((!sessionFileName) || (sessionFileName[0] == '\0'))
				return FALSE;

			Session session2Load;
			if (nppParam.loadSession(session2Load, sessionFileName, true))
			{
				size_t i = 0;
				for ( ; i < session2Load.nbMainFiles() ; )
				{
					const wchar_t *pFn = session2Load._mainViewFiles[i]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}

				for (size_t j = 0, len = session2Load.nbSubFiles(); j < len ; ++j)
				{
					const wchar_t *pFn = session2Load._subViewFiles[j]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_DECODESCI:
		{
			// convert to ASCII
			ScintillaEditView *pSci = nullptr;
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
			ScintillaEditView *pSci = nullptr;
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
				NMHDR	nmhdr{};
				nmhdr.code = NM_RCLICK;

				nmhdr.hwndFrom = (whichView == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();

				nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
				::SendMessage(hwnd, WM_NOTIFY, nmhdr.idFrom, reinterpret_cast<LPARAM>(&nmhdr));
			}
			return TRUE;
		}

		case NPPM_INTERNAL_CLOSEDOC:
		{
			// Close a document without switching to it
			int whichView = ((wParam != MAIN_VIEW) && (wParam != SUB_VIEW)) ? currentView() : static_cast<int32_t>(wParam);
			int index = static_cast<int32_t>(lParam);
			
			// Gotta switch to correct view to get the correct buffer ID
			switchEditViewTo(whichView);

			// Close the document
			fileClose(_pDocTab->getBufferByIndex(index), whichView);
			
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
			const wchar_t* verStr = VERSION_INTERNAL_VALUE;
			wchar_t mainVerStr[16]{};
			wchar_t auxVerStr[16]{};
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
				mainVer = _wtoi(mainVerStr);

			if (auxVerStr[0])
				auxVer = _wtoi(auxVerStr);

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
			wstring cmdLineString = nppParam.getCmdLineString();

			if (lParam != 0)
			{
				if (cmdLineString.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<wchar_t*>(lParam), cmdLineString.c_str());
			}
			return cmdLineString.length();
		}

		case NPPM_CREATELEXER:
		{
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			const char* lexer_name = wmc.wchar2char(reinterpret_cast<wchar_t*>(lParam), CP_ACP);
			return (LRESULT) CreateLexer(lexer_name);
		}

		case WM_FRSAVE_INT:
		{
			_macro.push_back(recordedMacroStep(static_cast<int32_t>(wParam), 0, lParam, NULL, recordedMacroStep::mtSavedSnR));
			break;
		}

		case WM_FRSAVE_STR:
		{
			_macro.push_back(recordedMacroStep(static_cast<int32_t>(wParam), 0, 0, reinterpret_cast<const char *>(lParam), recordedMacroStep::mtSavedSnR));
			break;
		}

		case WM_MACRODLGRUNMACRO:
		{
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				int times = _runMacroDlg.isMulti() ? _runMacroDlg.getTimes() : -1;

				int counter = 0;
				intptr_t lastLine = _pEditView->execute(SCI_GETLINECOUNT) - 1;
				intptr_t currLine = _pEditView->getCurrentLineNumber();
				int indexMacro = _runMacroDlg.getMacro2Exec();
				intptr_t deltaLastLine = 0;
				intptr_t deltaCurrLine = 0;
				bool cursorMovedUp = false;

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
						if (counter > 2 && cursorMovedUp != (deltaCurrLine < 0) && deltaLastLine >= 0)
						{
							// the current line must be monotonically increasing or monotonically
							// decreasing. Otherwise we don't know that the loop will end,
							// unless the number of lines is decreasing with every iteration.
							break;
						}
						cursorMovedUp = deltaCurrLine < 0;
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

		case NPPM_DESTROYSCINTILLAHANDLE_DEPRECATED:
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
			wchar_t *str2set = reinterpret_cast<wchar_t *>(lParam);
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
				return static_cast<LRESULT>(NULL);
		}

		case NPPM_LOADSESSION:
		{
			fileLoadSession(reinterpret_cast<const wchar_t *>(lParam));
			return TRUE;
		}

		case NPPM_SAVECURRENTSESSION:
		{
			return (LRESULT)fileSaveSession(0, NULL, reinterpret_cast<const wchar_t *>(lParam));
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

		case NPPM_INTERNAL_SETNPC:
		{
			const auto& svp = nppParam.getSVP();
			const bool isFromIncCcUniEolCtrl = wParam == IDC_CHECK_NPC_INCLUDECCUNIEOL;
			if (isFromIncCcUniEolCtrl || svp._npcIncludeCcUniEol)
			{
				const bool isShown = svp._ccUniEolShow;
				_mainEditView.showCcUniEol(isShown);
				_subEditView.showCcUniEol(isShown);
			}

			if (!isFromIncCcUniEolCtrl)
			{
				const bool isShown = svp._npcShow;
				_mainEditView.showNpc(isShown);
				_subEditView.showNpc(isShown);
				_findReplaceDlg.updateFinderScintillaForNpc();
			}

			return TRUE;
		}

		case NPPM_INTERNAL_SETMULTISELCTION:
		{
			ScintillaViewParams& svp = const_cast<ScintillaViewParams&>(nppParam.getSVP());
			_mainEditView.execute(SCI_SETMULTIPLESELECTION, svp._multiSelection);
			_subEditView.execute(SCI_SETMULTIPLESELECTION, svp._multiSelection);
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
			return TabBarPlus::isReduced() ? TRUE : FALSE;
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
						return static_cast<LRESULT>(NULL);
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
							return static_cast<LRESULT>(NULL);
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
				HWND activeViewHwnd = reinterpret_cast<HWND>(wParam);

				if ((activeViewHwnd == _mainEditView.getHSelf()) || (activeViewHwnd == _subEditView.getHSelf()))
				{
					if (activeViewHwnd == _mainEditView.getHSelf())
						switchEditViewTo(MAIN_VIEW);
					else
						switchEditViewTo(SUB_VIEW);

					ContextMenu scintillaContextmenu;
					
					std::vector<MenuItemUnit>& tmp = nppParam.getContextMenuItems();
					bool copyLink = (_pEditView->getSelectedTextCount() == 0) && _pEditView->getIndicatorRange(URL_INDIC);
					scintillaContextmenu.create(hwnd, tmp, _mainMenuHandle, copyLink);
					
					POINT p{};
					p.x = GET_X_LPARAM(lParam);
					p.y = GET_Y_LPARAM(lParam);
					if ((p.x == -1) && (p.y == -1))
					{
						// context menu activated via keyboard; pop up at text caret position
						auto caretPos = _pEditView->execute(SCI_GETCURRENTPOS);
						p.x = static_cast<LONG>(_pEditView->execute(SCI_POINTXFROMPOSITION, 0, caretPos));
						p.y = static_cast<LONG>(_pEditView->execute(SCI_POINTYFROMPOSITION, 0, caretPos));
						::ClientToScreen(activeViewHwnd, &p);
					}
					scintillaContextmenu.display(p);

					return TRUE;
				}
			}

			return ::DefWindowProc(hwnd, message, wParam, lParam);
		}

		case WM_NOTIFY:
		{
			const auto lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (lpnmhdr->hwndFrom == _toolBar.getHSelf())
			{
				switch (lpnmhdr->code)
				{
					case NM_CUSTOMDRAW:
					{
						auto nmtbcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
						static int roundCornerValue = 0;

						switch (nmtbcd->nmcd.dwDrawStage)
						{
							case CDDS_PREPAINT:
							{
								LRESULT lr = CDRF_DODEFAULT;
								if (NppDarkMode::isEnabled())
								{
									if (NppDarkMode::isWindows11())
									{
										roundCornerValue = 5;
									}

									::FillRect(nmtbcd->nmcd.hdc, &nmtbcd->nmcd.rc, NppDarkMode::getDarkerBackgroundBrush());
									lr |= CDRF_NOTIFYITEMDRAW;
								}

								return lr;
							}

							case CDDS_ITEMPREPAINT:
							{
								nmtbcd->hbrMonoDither = NppDarkMode::getBackgroundBrush();
								nmtbcd->hbrLines = NppDarkMode::getEdgeBrush();
								nmtbcd->hpenLines = NppDarkMode::getEdgePen();
								nmtbcd->clrText = NppDarkMode::getTextColor();
								nmtbcd->clrTextHighlight = NppDarkMode::getTextColor();
								nmtbcd->clrBtnFace = NppDarkMode::getBackgroundColor();
								nmtbcd->clrBtnHighlight = NppDarkMode::getSofterBackgroundColor();
								nmtbcd->clrHighlightHotTrack = NppDarkMode::getHotBackgroundColor();
								nmtbcd->nStringBkMode = TRANSPARENT;
								nmtbcd->nHLStringBkMode = TRANSPARENT;

								RECT rcItem{ nmtbcd->nmcd.rc };
								RECT rcDrop{};

								TBBUTTONINFO tbi{};
								tbi.cbSize = sizeof(TBBUTTONINFO);
								tbi.dwMask = TBIF_STYLE;
								::SendMessage(lpnmhdr->hwndFrom, TB_GETBUTTONINFO, nmtbcd->nmcd.dwItemSpec, reinterpret_cast<LPARAM>(&tbi));
								const bool isDropDown = (tbi.fsStyle & BTNS_DROPDOWN) == BTNS_DROPDOWN;
								if (isDropDown)
								{
									WPARAM idx = ::SendMessage(lpnmhdr->hwndFrom, TB_COMMANDTOINDEX, nmtbcd->nmcd.dwItemSpec, 0);
									::SendMessage(lpnmhdr->hwndFrom, TB_GETITEMDROPDOWNRECT, idx, reinterpret_cast<LPARAM>(&rcDrop));

									rcItem.right = rcDrop.left;
								}

								if ((nmtbcd->nmcd.uItemState & CDIS_HOT) == CDIS_HOT)
								{
									auto holdBrush = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getHotBackgroundBrush());
									auto holdPen = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getHotEdgePen());

									::RoundRect(nmtbcd->nmcd.hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom, roundCornerValue, roundCornerValue);

									if (isDropDown)
									{
										::RoundRect(nmtbcd->nmcd.hdc, rcDrop.left, rcDrop.top, rcDrop.right, rcDrop.bottom, roundCornerValue, roundCornerValue);
									}

									::SelectObject(nmtbcd->nmcd.hdc, holdBrush);
									::SelectObject(nmtbcd->nmcd.hdc, holdPen);

									nmtbcd->nmcd.uItemState &= ~(CDIS_CHECKED | CDIS_HOT);
								}
								else if ((nmtbcd->nmcd.uItemState & CDIS_CHECKED) == CDIS_CHECKED)
								{
									auto holdBrush = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getSofterBackgroundBrush());
									auto holdPen = ::SelectObject(nmtbcd->nmcd.hdc, NppDarkMode::getEdgePen());

									::RoundRect(nmtbcd->nmcd.hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom, roundCornerValue, roundCornerValue);

									if (isDropDown)
									{
										::RoundRect(nmtbcd->nmcd.hdc, rcDrop.left, rcDrop.top, rcDrop.right, rcDrop.bottom, roundCornerValue, roundCornerValue);
									}

									::SelectObject(nmtbcd->nmcd.hdc, holdBrush);
									::SelectObject(nmtbcd->nmcd.hdc, holdPen);

									nmtbcd->nmcd.uItemState &= ~CDIS_CHECKED;
								}

								LRESULT lr = TBCDRF_USECDCOLORS;
								if ((nmtbcd->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED)
								{
									lr |= TBCDRF_NOBACKGROUND;
								}

								if (isDropDown)
								{
									lr |= CDRF_NOTIFYPOSTPAINT;
								}

								return lr;
							}

							case CDDS_ITEMPOSTPAINT:
							{

								const UINT dpi = DPIManagerV2::getDpiForWindow(hwnd);
								LOGFONT lf{ DPIManagerV2::getDefaultGUIFontForDpi(dpi) };
								HFONT hFont = CreateFontIndirect(&lf);
								auto holdFont = static_cast<HFONT>(::SelectObject(nmtbcd->nmcd.hdc, hFont));

								RECT rcArrow{};
								WPARAM idx = ::SendMessage(lpnmhdr->hwndFrom, TB_COMMANDTOINDEX, nmtbcd->nmcd.dwItemSpec, 0);
								::SendMessage(lpnmhdr->hwndFrom, TB_GETITEMDROPDOWNRECT, idx, reinterpret_cast<LPARAM>(&rcArrow));
								rcArrow.left += DPIManagerV2::scale(1, dpi);
								rcArrow.bottom -= DPIManagerV2::scale(3, dpi);

								COLORREF clrArrow = NppDarkMode::getTextColor();

								::SetBkMode(nmtbcd->nmcd.hdc, TRANSPARENT);
								::SetTextColor(nmtbcd->nmcd.hdc, clrArrow);
								::DrawText(nmtbcd->nmcd.hdc, L"⏷", -1, &rcArrow, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
								::SelectObject(nmtbcd->nmcd.hdc, holdFont);
								::DeleteObject(hFont);

								return CDRF_DODEFAULT;
							}


							default:
								break;
						}

						return CDRF_DODEFAULT;
					}

					case TBN_DROPDOWN:
					{
						auto lpnmtb = reinterpret_cast<LPNMTOOLBARW>(lParam);
						switch (lpnmtb->iItem)
						{
							case IDM_VIEW_ALL_CHARACTERS:
							{
								auto cmdIDs = { IDM_VIEW_TAB_SPACE, IDM_VIEW_EOL, IDM_VIEW_NPC, IDM_VIEW_NPC_CCUNIEOL, 0, IDM_VIEW_ALL_CHARACTERS };
								notifyTBShowMenu(lpnmtb, "view-showSymbol", cmdIDs);
								return TBDDRET_DEFAULT;
							}

							default:
								break;
						}
						return TBDDRET_NODEFAULT;
					}

					case NM_RCLICK:
					{
						auto lpnmtb = reinterpret_cast<LPNMTOOLBARW>(lParam);
						switch (lpnmtb->iItem)
						{
							case IDM_VIEW_ALL_CHARACTERS:
							{
								return notifyTBShowMenu(lpnmtb, "view-showSymbol");
							}

							default:
								break;
						}
						return FALSE;
					}

					default:
						return FALSE;
				}
			}

			SCNotification *notification = reinterpret_cast<SCNotification *>(lParam);

			if (notification->nmhdr.code == SCN_UPDATEUI)
			{
				checkClipboard();
				checkUndoState();
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

			if (lParam == SUB_VIEW) // priorityView is sub view, so we search in sub view firstly
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
			else // (lParam == SUB_VIEW): priorityView is main view, so we search in main view firstly
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
			return MainFileManager.getFileNameFromBuffer(reinterpret_cast<BufferID>(wParam), reinterpret_cast<wchar_t *>(lParam));
		}

		case WM_ACTIVATE:
		{
			if (wParam != WA_INACTIVE && _pEditView && _pNonEditView)
			{
				_pEditView->grabFocus();
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
			bool doChangePanel = (wParam == TRUE);

			//reset styler for change in Stylers.xml
			_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
			_mainEditView.performGlobalStyles();

			_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());
			_subEditView.performGlobalStyles();

			int urlAction = nppParam.getNppGUI()._styleURL;
			if (urlAction != urlDisable)
			{
				if (_mainEditView.getCurrentBuffer()->allowClickableLink())
					addHotSpot(&_mainEditView);

				if (_subEditView.getCurrentBuffer()->allowClickableLink())
					addHotSpot(&_subEditView);
			}

			_findReplaceDlg.updateFinderScintilla();
			
			_findReplaceDlg.redraw();

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

			if (doChangePanel) // Theme change
			{
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

				NppDarkMode::updateTreeViewStylePrev();
			}

			if (_pDocMap)
				_pDocMap->setSyntaxHiliting();

			// Notify plugins of update to styles xml
			SCNotification scnN{};
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

		case NPPM_INTERNAL_WINDOWSSESSIONEXIT:
		{
			int answer = _nativeLangSpeaker.messageBox("WindowsSessionExit",
				_pPublicInterface->getHSelf(),
				L"Windows session is about to be terminated but you have some data unsaved. Do you want to exit Notepad++ now?",
				L"Notepad++ - Windows session exit",
				MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);
			if (answer == IDYES)
				::PostMessage(_pPublicInterface->getHSelf(), WM_CLOSE, 0, 0);
			return TRUE;
		}

		case NPPM_INTERNAL_EXTERNALLEXERBUFFER:
		{
			// A buffer is just applied to an external lexer, let's send a notification to lexer plugin 
			// so the concerning plugin can manage it (associate the buffer & lexer instance).
			SCNotification scnN{};
			scnN.nmhdr.code = NPPN_EXTERNALLEXERBUFFER;
			scnN.nmhdr.hwndFrom = hwnd;
			scnN.nmhdr.idFrom = lParam;
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case NPPM_INTERNAL_CHECKUNDOREDOSTATE:
		{
			checkClipboard();
			checkUndoState();
			return TRUE;
		}

		case NPPM_INTERNAL_LINECUTCOPYWITHOUTSELECTION:
		{
			if (nppParam.getSVP()._lineCopyCutWithoutSelection) // "Disable Copy Cut Line Without Selection" is just unchecked: From NOT using it to using this feature
			{
				// Enable Copy & Cut unconditionally
				enableCommand(IDM_EDIT_CUT, true, MENU | TOOLBAR);
				enableCommand(IDM_EDIT_COPY, true, MENU | TOOLBAR);

			}
			else // "Disable Copy Cut Line Without Selection" is just checked: From using this feature to NOT using it
			{
				// Check the current selection to disable/enable Copy & Cut
				bool hasSelection = _pEditView->hasSelection();
				enableCommand(IDM_EDIT_CUT, hasSelection, MENU | TOOLBAR);
				enableCommand(IDM_EDIT_COPY, hasSelection, MENU | TOOLBAR);
			}
			return TRUE;
		}

		case WM_QUERYENDSESSION:
		{
			// app should return TRUE or FALSE immediately upon receiving this message,
			// and defer any cleanup operations until it receives WM_ENDSESSION (with WPARAM TRUE)

			// for a bigger tidy-up/save operations we can kick off a background thread here to prepare for shutdown
			// and when we get the WM_ENDSESSION TRUE, we wait there until that background operation completes
			// before telling the system, "ok, you can shut down now...", i.e. returning 0 there

			// whatever we do from here - make sure that it is ok for the operation to occur even if the shutdown
			// is then subsequently canceled

			// here we could also display a prompt to ask the users whether they want to save their unsaved changes etc.,
			// but in practice, this is usually not a good idea because if we do not respond to this message
			// after a few seconds (e.g. user is away from PC...), the system will shut down without us

			bool isFirstQueryEndSession = !nppParam.isEndSessionStarted();
			bool isForcedShuttingDown = (lParam & ENDSESSION_CRITICAL);

			nppParam.endSessionStart();
			if (isForcedShuttingDown)
				nppParam.makeEndSessionCritical();

			if (nppParam.doNppLogNulContentCorruptionIssue())
			{
				wstring issueFn = nppLogNulContentCorruptionIssue;
				issueFn += L".log";
				wstring nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);

				string wmqesType = std::to_string(lParam);
				if (lParam == 0)
				{
					wmqesType += " - ordinary system shutdown/restart";
				}
				else
				{
					// the lParam here is a bit mask, it can be one or more of the following values
					if (lParam & ENDSESSION_CLOSEAPP)
						wmqesType += " - ENDSESSION_CLOSEAPP";
					if (lParam & ENDSESSION_CRITICAL)
						wmqesType += " - ENDSESSION_CRITICAL";
					if (lParam & ENDSESSION_LOGOFF)
						wmqesType += " - ENDSESSION_LOGOFF";
				}
				string msg = "WM_QUERYENDSESSION (lParam: " + wmqesType + ") =====================================";
				writeLog(nppIssueLog.c_str(), msg.c_str());
			}

			if (::IsWindowEnabled(hwnd))
			{
				if (MainFileManager.getNbDirtyBuffers() > 0)
				{
					// we have unsaved filebuffer(s), give the user a chance to respond
					// (but only for a non-critical OS restart/shutdown and while the Notepad++ backup mode is OFF)
					if (!isForcedShuttingDown && isFirstQueryEndSession && !nppParam.getNppGUI().isSnapshotMode())
					{
						// if Notepad++ has been minimized or invisible, we need to show it 1st
						if (::IsIconic(hwnd))
						{
							::ShowWindow(hwnd, SW_RESTORE);
						}
						else
						{
							if (!::IsWindowVisible(hwnd))
							{
								// systray etc...
								::ShowWindow(hwnd, SW_SHOW);
								::SendMessage(hwnd, WM_SIZE, 0, 0);	// to make window fit (specially to show tool bar.)
							}
						}
						::PostMessage(hwnd, NPPM_INTERNAL_WINDOWSSESSIONEXIT, 0, 0); // posting will not block us here
						return FALSE; // request abort of the shutdown 
					}
				}
			}
			else
			{
				// we probably have a blocking modal-window like MessageBox (e.g. the "Reload" or "Keep non existing file")
				if (!isForcedShuttingDown && isFirstQueryEndSession)
					return FALSE; // request abort of the shutdown (for a non-critical one we can give the user a chance to solve whatever is needed)

				// here is the right place to unblock the modal-dlg blocking the main Notepad++ wnd, because then it will be too late
				// to do so at the WM_ENDSESSION time (for that we need this thread message queue...)

				// in most cases we will need to take care and programmatically close such dialogs in order to exit gracefully,
				// otherwise the Notepad++ most probably crashes itself without any tidy-up

				string strLog = "Main Notepad++ wnd is disabled by (an active modal-dlg?):  ";
				char szBuf[MAX_PATH + 128] = { 0 };

				HWND hActiveWnd = ::GetActiveWindow();
				if (hActiveWnd)
				{
					if (::GetWindowTextA(hActiveWnd, szBuf, _countof(szBuf)))
						strLog += szBuf;
					::SendMessage(hActiveWnd, WM_CLOSE, 0, 0);
				}
				else
				{
					// no active child window, so it is most probably the system dialog box class #32770 (used e.g. by the MessageBox WINAPI)
					// - so our main hwnd here is not the PARENT but OWNER of that top-level window
					hActiveWnd = ::GetLastActivePopup(hwnd);
					if (hActiveWnd)
					{
						if (::GetWindowTextA(hActiveWnd, szBuf, _countof(szBuf)))
							strLog += szBuf;
						::GetClassNameA(hActiveWnd, szBuf, _countof(szBuf));
						if (lstrcmpiA("#32770", szBuf) == 0)
							strLog += " (MessageBox)";
						// we cannot use here the usual sending of the WM_CLOSE as it will not always work (e.g. for a MB_YESNO MessageBox)
						if (!::EndDialog(hActiveWnd, 0))
						{
							strLog += "  -> EndDialog failed with ErrorCode: ";
							strLog += std::to_string(::GetLastError());
							// last attempt
							::SendMessage(hActiveWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
						}
					}
					else
					{
						strLog += "???";
					}
				}

				// re-test
				if (::IsWindowEnabled(hwnd))
					strLog += "  -> Main Notepad++ wnd has been successfully reenabled.";

				if (nppParam.doNppLogNulContentCorruptionIssue())
				{
					wstring issueFn = nppLogNulContentCorruptionIssue;
					issueFn += L".log";
					wstring nppIssueLog = nppParam.getUserPath();
					pathAppend(nppIssueLog, issueFn);
					writeLog(nppIssueLog.c_str(), strLog.c_str());
				}
			}

			// NOTE: This should be the last possible place to eventually register Notepad++ for the app-restart OS feature,
			//       but unfortunately it doesn't work here.

			return TRUE; // nowadays, with the monstrous Win10+ Windows Update behind, is futile to try to interrupt the shutdown by returning FALSE here
						 // (if one really needs so, there is the ShutdownBlockReasonCreate WINAPI for the rescue ...)
		}

		case WM_ENDSESSION:
		{
			// this message informs our app whether the session is really ending

			if (nppParam.doNppLogNulContentCorruptionIssue())
			{
				wstring issueFn = nppLogNulContentCorruptionIssue;
				issueFn += L".log";
				wstring nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);

				string wmesType = std::to_string(lParam);
				if (lParam == 0)
				{
					wmesType += " - ordinary system shutdown/restart";
				}
				else
				{
					// the lParam here is a bit mask, it can be one or more of the following values
					if (lParam & ENDSESSION_CLOSEAPP)
						wmesType += " - ENDSESSION_CLOSEAPP";
					if (lParam & ENDSESSION_CRITICAL)
						wmesType += " - ENDSESSION_CRITICAL";
					if (lParam & ENDSESSION_LOGOFF)
						wmesType += " - ENDSESSION_LOGOFF";
				}
				string msg = "WM_ENDSESSION (wParam: ";
				if (wParam)
					msg += "TRUE, lParam: ";
				else
					msg += "FALSE, lParam: ";
				msg += wmesType + ")";

				writeLog(nppIssueLog.c_str(), msg.c_str());
			}

			if (wParam == FALSE)
			{
				// the session is not being ended after all
				// - it happens when either the Notepad++ returns FALSE to non-critical WM_QUERYENDSESSION or any other app with higher shutdown level
				//   than Notepad++ (app shuttdown order can be checked by the GetProcessShutdownParameters WINAPI)
				// - we will not try to reset back our nppParam _isEndSessionStarted flag somehow, because of we should now that there was already
				//   a previous shutdown attempt, otherwise we could stubbornly repeat returning FALSE for the next WM_QUERYENDSESSION and
				//   the system will terminate us
				return 0; // return here and do not continue to the WM_CLOSE part
			}
			else
			{
				// the session is being ended, it can end ANY TIME after all apps running have returned from processing this message,
				// so DO NOT e.g. Send/Post any message from here onwards!!!
				nppParam.endSessionStart(); // ensure
				nppParam.makeEndSessionCritical(); // set our exit-flag to critical even if the bitmask has not the ENDSESSION_CRITICAL set
				// do not return 0 here and continue to the Notepad++ standard WM_CLOSE code-part (no verbose GUI there this time!!!)
				[[fallthrough]];
			}
		} // case WM_ENDSESSION:

		case WM_CLOSE:
		{
			if (nppParam.doNppLogNulContentCorruptionIssue() && nppParam.isEndSessionStarted() && (message == WM_CLOSE))
			{
				wstring issueFn = nppLogNulContentCorruptionIssue;
				issueFn += L".log";
				wstring nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);
				writeLog(nppIssueLog.c_str(), "WM_CLOSE (isEndSessionStarted == true)");
			}

			if (_pPublicInterface->isPrelaunch())
			{
				SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
			else
			{
				SCNotification scnN{};
				scnN.nmhdr.hwndFrom = hwnd;
				scnN.nmhdr.idFrom = 0;
				scnN.nmhdr.code = NPPN_BEFORESHUTDOWN;
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
					_lastRecentFileList.setLock(false);	//only unlock when the session is remembered

				if (!saveProjectPanelsParams()) allClosed = false; //writeProjectPanelsSettings
				saveFileBrowserParam();
				saveColumnEditorParams();

				if (!allClosed && !nppParam.isEndSessionCritical())
				{
					// cancelled by user
					scnN.nmhdr.code = NPPN_CANCELSHUTDOWN;
					_pluginsManager.notify(&scnN);
					if (isSnapshotMode)
						::LockWindowUpdate(NULL);

					if (!::IsWindowVisible(hwnd))
					{
						// Notepad++ probably has not been restored from the systray
						// - as its tray-icon was removed before, we have to show the app window otherwise we end up with no-GUI state
						::ShowWindow(hwnd, SW_SHOW);
						::SendMessage(hwnd, WM_SIZE, 0, 0);
					}

					return 0; // abort quitting
				}

				// from this point on the Notepad++ exit is inevitable
				g_bNppExitFlag.store(true); // thread-safe op
				// currently it is used only in the Notepad_plus::backupDocument worker thread,
				// use it in such a thread like:	if (g_bNppExitFlag.load()) -> finish work of & exit the thread

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

				if (!_isNppSessionSavedAtExit)
				{
					_isNppSessionSavedAtExit = true; // prevents emptying of the session.xml file on another WM_ENDSESSION or WM_CLOSE

					//
					// saving session.xml
					//
					if (nppgui._rememberLastSession && !nppgui._isCmdlineNosessionActivated)
						saveSession(currentSession);

					//
					// saving session.xml into loaded session if a saved session is loaded and saveLoadedSessionOnExit option is enabled
					//
					wstring loadedSessionFilePath = nppParam.getLoadedSessionFilePath();
					if (!loadedSessionFilePath.empty() && doesFileExist(loadedSessionFilePath.c_str()))
						nppParam.writeSession(currentSession, loadedSessionFilePath.c_str());
				}

				// write settings on cloud if enabled, if the settings files don't exist
				if (!nppgui._cloudPath.empty() && nppParam.isCloudPathChanged())
				{
					bool isOK = nppParam.writeSettingsFilesOnCloudForThe1stTime(nppgui._cloudPath);
					if (!isOK)
					{
						_nativeLangSpeaker.messageBox("SettingsOnCloudError",
							hwnd,
							L"It seems the path of settings on cloud is set on a read only drive,\ror on a folder needed privilege right for writing access.\rYour settings on cloud will be canceled. Please reset a coherent value via Preference dialog.",
							L"Settings on Cloud",
							MB_OK | MB_APPLMODAL);
						nppParam.removeCloudChoice();
					}
				}

				if (isSnapshotMode)
					::LockWindowUpdate(NULL);

				//Sends WM_DESTROY, Notepad++ will end
				::DestroyWindow(hwnd);

				if (!nppParam.isEndSessionCritical())
				{
					wstring updaterFullPath = nppParam.getWingupFullPath();
					if (!updaterFullPath.empty())
					{
						Process updater(updaterFullPath.c_str(), nppParam.getWingupParams().c_str(), nppParam.getWingupDir().c_str());
						updater.run(nppParam.shouldDoUAC());
					}
				}
			}

			return 0; // both WM_CLOSE and a possible WM_ENDSESSION should return 0
		}

		case WM_DESTROY:
		{
			if (nppParam.isEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
			{
				wstring issueFn = nppLogNulContentCorruptionIssue;
				issueFn += L".log";
				wstring nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);
				writeLog(nppIssueLog.c_str(), "WM_DESTROY (isEndSessionStarted == true)");
			}

			killAllChildren();
			::PostQuitMessage(0);
			_pPublicInterface->gNppHWND = NULL;
			return TRUE;
		}

		case NPPM_INTERNAL_RESTOREFROMTRAY:
		{
			// When mono instance, bring this one to front
			if (_pTrayIco != nullptr && _pTrayIco->isInTray())
			{
				// We are in tray, restore properly..
				::SendMessage(hwnd, NPPM_INTERNAL_MINIMIZED_TRAY, 0, WM_LBUTTONUP);
				return TRUE;
			}
			return FALSE;
		}

		case WM_SYSCOMMAND:
		{
			const NppGUI & nppgui = nppParam.getNppGUI();
			auto toTray = nppgui._isMinimizedToTray;
			if (((toTray == sta_minimize || toTray == sta_minimize_close || _pPublicInterface->isPrelaunch()) && (wParam == SC_MINIMIZE)) ||
				((toTray == sta_close || toTray == sta_minimize_close) && wParam == SC_CLOSE)
			)
			{
				if (nullptr == _pTrayIco)
				{
					HICON icon = nullptr;
					Notepad_plus_Window::loadTrayIcon(_pPublicInterface->getHinst(), &icon);
					_pTrayIco = new trayIconControler(hwnd, IDI_M30ICON, NPPM_INTERNAL_MINIMIZED_TRAY, icon, L"");
				}

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
					_pEditView->grabFocus();
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
					_nativeLangSpeaker.changeLangTrayIconContexMenu(hTrayIconMenu);
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
			_dockingManager.showDockableDlg(reinterpret_cast<wchar_t *>(lParam), SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMGETPLUGINHWNDBYNAME : //(const wchar_t *windowName, const wchar_t *moduleName)
		{
			if (!lParam)
				return static_cast<LRESULT>(NULL);

			wchar_t *moduleName = reinterpret_cast<wchar_t *>(lParam);
			wchar_t *windowName = reinterpret_cast<wchar_t *>(wParam);
			std::vector<DockingCont *> dockContainer = _dockingManager.getContainerInfo();

			for (size_t i = 0, len = dockContainer.size(); i < len ; ++i)
			{
				std::vector<tTbData *> tbData = dockContainer[i]->getDataOfAllTb();
				for (size_t j = 0, len2 = tbData.size() ; j < len2 ; ++j)
				{
					if (wcsicmp(moduleName, tbData[j]->pszModuleName) == 0)
					{
						if (!windowName)
							return (LRESULT)tbData[j]->hClient;

						if (wcsicmp(windowName, tbData[j]->pszName) == 0)
							return (LRESULT)tbData[j]->hClient;
					}
				}
			}
			return static_cast<LRESULT>(NULL);
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

		case NPPM_GETENABLETHEMETEXTUREFUNC_DEPRECATED:
		{
			return reinterpret_cast<LRESULT>(&EnableThemeDialogTexture);
		}

		case NPPM_GETPLUGINSCONFIGDIR:
		{
			wstring userPluginConfDir = nppParam.getUserPluginConfDir();
			if (lParam != 0)
			{
				if (userPluginConfDir.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<wchar_t *>(lParam), userPluginConfDir.c_str());

				// For the retro-compatibility
				return TRUE;
			}
			return userPluginConfDir.length();
		}

		case NPPM_GETPLUGINHOMEPATH:
		{
			wstring pluginHomePath = nppParam.getPluginRootDir();
			if (lParam != 0)
			{
				if (pluginHomePath.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<wchar_t *>(lParam), pluginHomePath.c_str());
			}
			return pluginHomePath.length();
		}

		case NPPM_GETSETTINGSONCLOUDPATH:
		{
			const NppGUI & nppGUI = nppParam.getNppGUI();
			wstring settingsOnCloudPath = nppGUI._cloudPath;
			if (lParam != 0)
			{
				if (settingsOnCloudPath.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				lstrcpy(reinterpret_cast<wchar_t *>(lParam), settingsOnCloudPath.c_str());
			}
			return settingsOnCloudPath.length();
		}

		case NPPM_SETLINENUMBERWIDTHMODE:
		{
			if (lParam != LINENUMWIDTH_DYNAMIC && lParam != LINENUMWIDTH_CONSTANT)
				return FALSE;

			ScintillaViewParams &svp = const_cast<ScintillaViewParams &>(nppParam.getSVP());
			svp._lineNumberMarginDynamicWidth = lParam == LINENUMWIDTH_DYNAMIC;
			::SendMessage(hwnd, NPPM_INTERNAL_LINENUMBER, 0, 0);

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

		case NPPM_ALLOCATESUPPORTED_DEPRECATED:
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

		case NPPM_ALLOCATEINDICATOR:
		{
			return _pluginsManager.allocateIndicator(static_cast<int32_t>(wParam), reinterpret_cast<int *>(lParam));
		}

		case NPPM_GETTABCOLORID:
		{
			const auto view = static_cast<int>(wParam);
			auto tabIndex = static_cast<int>(lParam);

			auto colorId = -1;  // no color (or unknown)

			auto pDt = _pDocTab;  // active view
			if (view == MAIN_VIEW)
			{
				pDt = &_mainDocTab;
			}
			else if (view == SUB_VIEW)
			{
				pDt = &_subDocTab;
			}

			if (tabIndex == -1)
			{
				tabIndex = pDt->getCurrentTabIndex();
			}
			
			if ((tabIndex >= 0) && (tabIndex < static_cast<int>(pDt->nbItem())))
			{
				colorId = pDt->getIndividualTabColourId(tabIndex);
			}
			
			return colorId;
		}

		case NPPM_SETUNTITLEDNAME:
		{
			return fileRenameUntitledPluginAPI(reinterpret_cast<BufferID>(wParam), reinterpret_cast<const wchar_t*>(lParam));
		}

		case NPPM_GETBOOKMARKID:
		{
			return MARK_BOOKMARK;
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

		case NPPM_INTERNAL_NPCFORMCHANGED:
		{
			_mainEditView.setNpcAndCcUniEOL();
			_subEditView.setNpcAndCcUniEOL();

			const auto& svp = NppParameters::getInstance().getSVP();
			if (svp._npcShow)
			{
				_findReplaceDlg.updateFinderScintillaForNpc(true);
			}

			return TRUE;
		}

		case NPPM_INTERNAL_ENABLECHANGEHISTORY:
		{
			static bool stopActionUntilNextSession = false;

			const ScintillaViewParams& svp = nppParam.getSVP();

			int enabledCHFlag = SC_CHANGE_HISTORY_DISABLED;
			if (svp._isChangeHistoryMarginEnabled || svp._isChangeHistoryIndicatorEnabled)
			{
				enabledCHFlag = SC_CHANGE_HISTORY_ENABLED;

				if  (svp._isChangeHistoryMarginEnabled)
					enabledCHFlag |= SC_CHANGE_HISTORY_MARKERS;

				if  (svp._isChangeHistoryIndicatorEnabled)
					enabledCHFlag |= SC_CHANGE_HISTORY_INDICATORS;
			}

			if (!stopActionUntilNextSession)
			{
				_mainEditView.execute(SCI_SETCHANGEHISTORY, enabledCHFlag);
				_subEditView.execute(SCI_SETCHANGEHISTORY, enabledCHFlag);

				if (enabledCHFlag == SC_CHANGE_HISTORY_DISABLED)
					stopActionUntilNextSession = true;

				_mainEditView.showChangeHistoryMargin(svp._isChangeHistoryMarginEnabled);
				_subEditView.showChangeHistoryMargin(svp._isChangeHistoryMarginEnabled);

				_mainEditView.redraw();
				_subEditView.redraw();

				enableCommand(IDM_SEARCH_CHANGED_PREV, svp._isChangeHistoryMarginEnabled || svp._isChangeHistoryIndicatorEnabled, MENU);
				enableCommand(IDM_SEARCH_CHANGED_NEXT, svp._isChangeHistoryMarginEnabled || svp._isChangeHistoryIndicatorEnabled, MENU);
				enableCommand(IDM_SEARCH_CLEAR_CHANGE_HISTORY, svp._isChangeHistoryMarginEnabled || svp._isChangeHistoryIndicatorEnabled, MENU);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_CLEANBRACEMATCH:
		{
			_mainEditView.execute(SCI_SETHIGHLIGHTGUIDE, 0);
			_subEditView.execute(SCI_SETHIGHLIGHTGUIDE, 0);
			_mainEditView.execute(SCI_BRACEBADLIGHT, WPARAM(-1));
			_subEditView.execute(SCI_BRACEBADLIGHT, WPARAM(-1));
			return TRUE;
		}

		case NPPM_INTERNAL_CLEANSMARTHILITING:
		{
			_mainEditView.clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);
			_subEditView.clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_SMART);
			return TRUE;
		}

		case NPPM_INTERNAL_CRLFLAUNCHSTYLECONF:
		{
			// Launch _configStyleDlg (create or display it)
			command(IDM_LANGSTYLE_CONFIG_DLG);

			// go into the section we need
			_configStyleDlg.goToSection(L"Global Styles:EOL custom color");

			return TRUE;
		}

		case NPPM_INTERNAL_NPCLAUNCHSTYLECONF:
		{
			// Launch _configStyleDlg (create or display it)
			command(IDM_LANGSTYLE_CONFIG_DLG);

			// go into the section we need
			wstring npcStr = L"Global Styles:";
			npcStr += g_npcStyleName;
			_configStyleDlg.goToSection(npcStr.c_str());

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
			//printStr(L"you've got me"));
			NppGUI & nppGUI = nppParam.getNppGUI();
			nppGUI._autoUpdateOpt._doAutoUpdate = false;
			return TRUE;
		}

		case NPPM_GETLANGUAGENAME:
		{
			wstring langName = getLangDesc((LangType)wParam, true);
			if (lParam)
				lstrcpy((LPTSTR)lParam, langName.c_str());
			return langName.length();
		}

		case NPPM_GETLANGUAGEDESC:
		{
			wstring langDesc = getLangDesc((LangType)wParam, false);
			if (lParam)
				lstrcpy((LPTSTR)lParam, langDesc.c_str());
			return langDesc.length();
		}

		case NPPM_GETEXTERNALLEXERAUTOINDENTMODE:
		{
			int index = nppParam.getExternalLangIndexFromName(reinterpret_cast<wchar_t*>(wParam));
			if (index < 0)
				return FALSE;

			*(reinterpret_cast<ExternalLexerAutoIndentMode*>(lParam)) = nppParam.getELCFromIndex(index)._autoIndentMode;
			return TRUE;
		}

		case NPPM_SETEXTERNALLEXERAUTOINDENTMODE:
		{
			int index = nppParam.getExternalLangIndexFromName(reinterpret_cast<wchar_t*>(wParam));
			if (index < 0)
				return FALSE;

			nppParam.getELCFromIndex(index)._autoIndentMode = static_cast<ExternalLexerAutoIndentMode>(lParam);
			return TRUE;
		}

		case NPPM_ISAUTOINDENTON:
		{
			return nppParam.getNppGUI()._maintainIndent;
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

		case NPPM_DARKMODESUBCLASSANDTHEME:
		{
			return static_cast<LRESULT>(NppDarkMode::autoSubclassAndThemePlugin(reinterpret_cast<HWND>(lParam), static_cast<ULONG>(wParam)));
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
			const wchar_t *appDataNpp = nppParam.getAppDataNppDir();
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
			const Style * pStyle = NppParameters::getInstance().getMiscStylerArray().findByName(L"Edge colour");
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

		case NPPM_INTERNAL_SET_TAB_SETTINGS:
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
			const UINT dpi = LOWORD(wParam);
			_toolBar.resizeIconsDpi(dpi);

			_mainDocTab.dpiManager().setDpi(dpi);
			_subDocTab.dpiManager().setDpi(dpi);
			_mainDocTab.setFont();
			_subDocTab.setFont();
			_mainDocTab.resizeIconsDpi();
			_subDocTab.resizeIconsDpi();
			_mainDocTab.setCloseBtnImageList();
			_subDocTab.setCloseBtnImageList();
			_mainDocTab.setPinBtnImageList();
			_subDocTab.setPinBtnImageList();
			::SendMessage(_pPublicInterface->getHSelf(), NPPM_INTERNAL_REDUCETABBAR, 0, 0);

			changeDocumentListIconSet(false);

			_statusBar.setPartWidth(STATUSBAR_DOC_SIZE, DPIManagerV2::scale(220, dpi));
			_statusBar.setPartWidth(STATUSBAR_CUR_POS, DPIManagerV2::scale(260, dpi));
			_statusBar.setPartWidth(STATUSBAR_EOF_FORMAT, DPIManagerV2::scale(110, dpi));
			_statusBar.setPartWidth(STATUSBAR_UNICODE_TYPE, DPIManagerV2::scale(120, dpi));
			_statusBar.setPartWidth(STATUSBAR_TYPING_MODE, DPIManagerV2::scale(30, dpi));

			return TRUE;
		}

		case NPPM_INTERNAL_UPDATECLICKABLELINKS:
		{
			if (wParam == 1)
			{
				removeAllHotSpot();
				return TRUE;
			}

			ScintillaEditView* pView = reinterpret_cast<ScintillaEditView*>(wParam);

			int urlAction = nppParam.getNppGUI()._styleURL;
			if (urlAction != urlDisable)
			{
				if (pView == NULL)
				{
					if (_pEditView->getCurrentBuffer()->allowClickableLink())
						addHotSpot(_pEditView);

					if (_pNonEditView->getCurrentBuffer()->allowClickableLink())
						addHotSpot(_pNonEditView);
				}
				else
				{
					if (pView->getCurrentBuffer()->allowClickableLink())
						addHotSpot(pView);
				}
			}
			return TRUE;
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
			wstring path = buf ? buf->getFullPathName() : L"";
			pathRemoveFileSpec(path);
			setWorkingDir(path.c_str());
			return TRUE;
		}

		case NPPM_INTERNAL_DOCMODIFIEDBYREPLACEALL:
		{
			Buffer* currentBuf = _pEditView->getCurrentBuffer();
			if (wParam == reinterpret_cast<WPARAM>(currentBuf))
			{
				int urlAction = nppParam.getNppGUI()._styleURL;
				if (urlAction != urlDisable && currentBuf->allowClickableLink())
				{
					addHotSpot(_pEditView);
				}
			}

			SCNotification scnN{};
			scnN.nmhdr.code = NPPN_GLOBALMODIFIED;
			scnN.nmhdr.hwndFrom = reinterpret_cast<void*>(wParam);
			scnN.nmhdr.idFrom = 0;
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case NPPM_GETNATIVELANGFILENAME:
		{
			const char* fn = _nativeLangSpeaker.getFileName();

			if (!fn) return 0;

			string fileName = fn;
			if (lParam != 0)
			{
				if (fileName.length() >= static_cast<size_t>(wParam))
				{
					return 0;
				}
				strcpy(reinterpret_cast<char*>(lParam), fileName.c_str());
			}
			return fileName.length();
		}

		case NPPM_ADDSCNMODIFIEDFLAGS:
		{
			nppParam.addScintillaModEventMask(static_cast<unsigned long>(lParam));

			auto newModEventMask = nppParam.getScintillaModEventMask();
			_mainEditView.execute(SCI_SETMODEVENTMASK, newModEventMask);
			_subEditView.execute(SCI_SETMODEVENTMASK, newModEventMask);
			return TRUE;
		}

		case NPPM_INTERNAL_HILITECURRENTLINE:
		{
			const ScintillaViewParams& svp = nppParam.getSVP();

			const COLORREF bgColour{ nppParam.getCurLineHilitingColour() };
			const LPARAM frameWidth{ (svp._currentLineHiliteMode == LINEHILITE_FRAME) ? svp._currentLineFrameWidth : 0 };

			if (svp._currentLineHiliteMode != LINEHILITE_NONE)
			{
				_mainEditView.setElementColour(SC_ELEMENT_CARET_LINE_BACK, bgColour);
				_subEditView.setElementColour(SC_ELEMENT_CARET_LINE_BACK, bgColour);
			}
			else
			{
				_mainEditView.execute(SCI_RESETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, 0);
				_subEditView.execute(SCI_RESETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, 0);
			}

			_mainEditView.execute(SCI_SETCARETLINEFRAME, frameWidth);
			_subEditView.execute(SCI_SETCARETLINEFRAME, frameWidth);
		}
		break;

		case NPPM_INTERNAL_LINENUMBER:
		case NPPM_INTERNAL_SYMBOLMARGIN:
		{
			int margin;
			if (message == NPPM_INTERNAL_LINENUMBER)
				margin = ScintillaEditView::_SC_MARGE_LINENUMBER;
			else //if (message == IDM_VIEW_SYMBOLMARGIN)
				margin = ScintillaEditView::_SC_MARGE_SYMBOL;

			if (_mainEditView.hasMarginShowed(margin))
			{
				_mainEditView.showMargin(margin, false);
				_subEditView.showMargin(margin, false);
			}
			else
			{
				_mainEditView.showMargin(margin);
				_subEditView.showMargin(margin);
			}
		}
		break;

		case NPPM_INTERNAL_LWDEF:
		case NPPM_INTERNAL_LWALIGN:
		case NPPM_INTERNAL_LWINDENT:
		{
			int mode = (message == NPPM_INTERNAL_LWALIGN) ? SC_WRAPINDENT_SAME : \
				(message == NPPM_INTERNAL_LWINDENT) ? SC_WRAPINDENT_INDENT : SC_WRAPINDENT_FIXED;
			_mainEditView.execute(SCI_SETWRAPINDENTMODE, mode);
			_subEditView.execute(SCI_SETWRAPINDENTMODE, mode);
		}
		break;

		case NPPM_INTERNAL_FOLDSYMBOLSIMPLE:
		case NPPM_INTERNAL_FOLDSYMBOLARROW:
		case NPPM_INTERNAL_FOLDSYMBOLCIRCLE:
		case NPPM_INTERNAL_FOLDSYMBOLBOX:
		case NPPM_INTERNAL_FOLDSYMBOLNONE:
		{
			folderStyle fStyle = 
				(message == NPPM_INTERNAL_FOLDSYMBOLSIMPLE) ? FOLDER_STYLE_SIMPLE : \
				(message == NPPM_INTERNAL_FOLDSYMBOLARROW) ? FOLDER_STYLE_ARROW : \
				(message == NPPM_INTERNAL_FOLDSYMBOLCIRCLE) ? FOLDER_STYLE_CIRCLE : \
				(message == NPPM_INTERNAL_FOLDSYMBOLNONE) ? FOLDER_STYLE_NONE : FOLDER_STYLE_BOX;

			_mainEditView.setMakerStyle(fStyle);
			_subEditView.setMakerStyle(fStyle);
		}
		break;

		case NPPM_INTERNAL_TOOLBARREDUCE:
		{
			toolBarStatusType state = _toolBar.getState();

			if (state != TB_SMALL)
			{
				_toolBar.reduce();
			}
		}
		break;

		case NPPM_INTERNAL_TOOLBARENLARGE:
		{
			toolBarStatusType state = _toolBar.getState();

			if (state != TB_LARGE)
			{
				_toolBar.enlarge();
			}
		}
		break;

		case NPPM_INTERNAL_TOOLBARREDUCESET2:
		{
			toolBarStatusType state = _toolBar.getState();

			if (state != TB_SMALL2)
			{
				_toolBar.reduceToSet2();
			}
		}
		break;

		case NPPM_INTERNAL_TOOLBARENLARGESET2:
		{
			toolBarStatusType state = _toolBar.getState();

			if (state != TB_LARGE2)
			{
				_toolBar.enlargeToSet2();
			}
		}
		break;

		case NPPM_INTERNAL_TOOLBARSTANDARD:
		{
			toolBarStatusType state = _toolBar.getState();

			if (state != TB_STANDARD)
			{
				_toolBar.setToBmpIcons();
			}
		}
		break;

		case NPPM_INTERNAL_LOCKTABBAR:
		{
			bool isDrag = TabBarPlus::doDragNDropOrNot();
			TabBarPlus::doDragNDrop(!isDrag);
			break;
		}


		case NPPM_INTERNAL_DRAWINACIVETAB:
		{
			TabBarPlus::setDrawInactiveTab(!TabBarPlus::drawInactiveTab(), &_mainDocTab);
			break;
		}
		case NPPM_INTERNAL_DRAWTABTOPBAR:
		{
			TabBarPlus::setDrawTopBar(!TabBarPlus::drawTopBar(), &_mainDocTab);
			break;
		}

		case NPPM_INTERNAL_TABDBCLK2CLOSE:
		{
			TabBarPlus::setDbClk2Close(!TabBarPlus::isDbClk2Close());
			break;
		}

		case NPPM_INTERNAL_VERTICALTABBAR:
		{
			TabBarPlus::setVertical(!TabBarPlus::isVertical());
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}

		case NPPM_INTERNAL_MULTILINETABBAR:
		{
			TabBarPlus::setMultiLine(!TabBarPlus::isMultiLine());
			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			break;
		}

		case NPPM_INTERNAL_REDUCETABBAR:
		{
			TabBarPlus::setReduced(!TabBarPlus::isReduced(), &_mainDocTab);
			bool isReduceed = TabBarPlus::isReduced();

			//Resize the tab height
			int tabDpiDynamicalWidth = _mainDocTab.dpiManager().scale((TabBarPlus::drawTabCloseButton() || TabBarPlus::drawTabPinButton()) ? g_TabWidthButton : g_TabWidth);
			int tabDpiDynamicalHeight = _mainDocTab.dpiManager().scale(isReduceed ? g_TabHeight : g_TabHeightLarge);

			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);

			//change the font
			const auto& hf = _mainDocTab.getFont(isReduceed);
			if (hf)
			{
				::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
				::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, reinterpret_cast<WPARAM>(hf), MAKELPARAM(TRUE, 0));
			}

			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);

			_mainDocTab.refresh();
			_subDocTab.refresh();
			break;
		}

		case NPPM_INTERNAL_DRAWTABBARCLOSEBUTTON:
		{
			TabBarPlus::setDrawTabCloseButton(!TabBarPlus::drawTabCloseButton(), &_mainDocTab);

			bool drawTabPinButton = TabBarPlus::drawTabPinButton();
			bool drawTabCloseButton = TabBarPlus::drawTabCloseButton();

			if (drawTabCloseButton && drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(0);
				_mainDocTab.setTabPinButtonOrder(1);
				_subDocTab.setTabCloseButtonOrder(0);
				_subDocTab.setTabPinButtonOrder(1);
			}
			else if (!drawTabCloseButton && drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(-1);
				_mainDocTab.setTabPinButtonOrder(0);
				_subDocTab.setTabCloseButtonOrder(-1);
				_subDocTab.setTabPinButtonOrder(0);
			}
			else if (drawTabCloseButton && !drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(0);
				_mainDocTab.setTabPinButtonOrder(-1);
				_subDocTab.setTabCloseButtonOrder(0);
				_subDocTab.setTabPinButtonOrder(-1);
			}
			else //if (!drawTabCloseButton && !drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(-1);
				_mainDocTab.setTabPinButtonOrder(-1);
				_subDocTab.setTabCloseButtonOrder(-1);
				_subDocTab.setTabPinButtonOrder(-1);
			}

			// This part is just for updating (redraw) the tabs
			int tabDpiDynamicalHeight = _mainDocTab.dpiManager().scale(TabBarPlus::isReduced() ? g_TabHeight : g_TabHeightLarge);
			int tabDpiDynamicalWidth = _mainDocTab.dpiManager().scale(TabBarPlus::drawTabCloseButton() ? g_TabWidthButton : g_TabWidth);
			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);

			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);

			_mainDocTab.refresh();
			_subDocTab.refresh();
			break;
		}

		case NPPM_INTERNAL_DRAWTABBARPINBUTTON:
		{
			TabBarPlus::setDrawTabPinButton(!TabBarPlus::drawTabPinButton(), &_mainDocTab);

			bool drawTabPinButton = TabBarPlus::drawTabPinButton();
			bool drawTabCloseButton = TabBarPlus::drawTabCloseButton();

			if (!drawTabPinButton)
			{
				unPinnedForAllBuffers();
			}

			if (drawTabCloseButton && drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(0);
				_mainDocTab.setTabPinButtonOrder(1);
				_subDocTab.setTabCloseButtonOrder(0);
				_subDocTab.setTabPinButtonOrder(1);
			}
			else if (!drawTabCloseButton && drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(-1);
				_mainDocTab.setTabPinButtonOrder(0);
				_subDocTab.setTabCloseButtonOrder(-1);
				_subDocTab.setTabPinButtonOrder(0);
			}
			else if (drawTabCloseButton && !drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(0);
				_mainDocTab.setTabPinButtonOrder(-1);
				_subDocTab.setTabCloseButtonOrder(0);
				_subDocTab.setTabPinButtonOrder(-1);
			}
			else //if (!drawTabCloseButton && !drawTabPinButton)
			{
				_mainDocTab.setTabCloseButtonOrder(-1);
				_mainDocTab.setTabPinButtonOrder(-1);
				_subDocTab.setTabCloseButtonOrder(-1);
				_subDocTab.setTabPinButtonOrder(-1);
			}

			// This part is just for updating (redraw) the tabs
			int tabDpiDynamicalHeight = _mainDocTab.dpiManager().scale(TabBarPlus::isReduced() ? g_TabHeight : g_TabHeightLarge);
			int tabDpiDynamicalWidth = _mainDocTab.dpiManager().scale(TabBarPlus::drawTabPinButton() ? g_TabWidthButton : g_TabWidth);
			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), tabDpiDynamicalWidth, tabDpiDynamicalHeight);

			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			_mainDocTab.refresh();
			_subDocTab.refresh();
			return TRUE;
		}

		case NPPM_INTERNAL_DRAWINACTIVETABBARBUTTON:
		{
			::SendMessage(_mainDocTab.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);
			::SendMessage(_subDocTab.getHSelf(), NPPM_INTERNAL_REFRESHDARKMODE, 0, 0);

			::SendMessage(_pPublicInterface->getHSelf(), WM_SIZE, 0, 0);
			_mainDocTab.refresh();
			_subDocTab.refresh();

			return TRUE;
		}

		case NPPM_INTERNAL_HIDEMENURIGHTSHORTCUTS:
		{
			if (nppParam.getNppGUI()._hideMenuRightShortcuts)
			{
				int nbRemoved = 0;
				const int bufferSize = 64;
				wchar_t buffer[bufferSize];
				int nbItem = GetMenuItemCount(_mainMenuHandle);
				for (int i = nbItem - 1; i >= 0; --i)
				{
					::GetMenuStringW(_mainMenuHandle, i, buffer, bufferSize, MF_BYPOSITION);
					if (lstrcmp(buffer, L"✕") == 0 || lstrcmp(buffer, L"▼") == 0 || lstrcmp(buffer, L"＋") == 0)
					{
						::RemoveMenu(_mainMenuHandle, i, MF_BYPOSITION);
						++nbRemoved;
					}
					if (nbRemoved == 3)
						break;
				}
				if (nbRemoved > 0)
					::DrawMenuBar(hwnd);
			}
			return TRUE;
		}

		case NPPM_INTERNAL_RELOADFUNCTIONLIST:
		{
			if (_pFuncList && (!_pFuncList->isClosed()) && _pFuncList->isVisible())
			{
				_pFuncList->reload();
			}
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
