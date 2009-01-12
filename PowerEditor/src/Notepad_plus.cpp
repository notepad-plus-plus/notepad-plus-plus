//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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
#ifndef _WIN32_IE
#define _WIN32_IE 0x500
#endif

//#define INCLUDE_DEPRECATED_FEATURES 1

#include <shlwapi.h>
//#include "dbghelp.h"

#include "Notepad_plus.h"
#include "FileDialog.h"
#include "resource.h"
#include "printer.h"
#include "FileNameStringSplitter.h"
#include "lesDlgs.h"
#include "Utf8_16.h"
#include "regExtDlg.h"
#include "RunDlg.h"
#include "ShortcutMapper.h"
#include "preferenceDlg.h"
#include "TaskListDlg.h"
#include "xpm_icons.h"
#include <algorithm>
#include "xmlMatchedTagsHighlighter.h"

const TCHAR Notepad_plus::_className[32] = TEXT("Notepad++");
HWND Notepad_plus::gNppHWND = NULL;
const char *urlHttpRegExpr = "http://[a-z0-9_\\-\\+.:?&@=/%#]*";

int docTabIconIDs[] = {IDI_SAVED_ICON, IDI_UNSAVED_ICON, IDI_READONLY_ICON};
enum tb_stat {tb_saved, tb_unsaved, tb_ro};

#define DIR_LEFT true
#define DIR_RIGHT false

struct SortTaskListPred
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

Notepad_plus::Notepad_plus(): Window(), _mainWindowStatus(0), _pDocTab(NULL), _pEditView(NULL),
	_pMainSplitter(NULL), _isfullScreen(false),
    _recordingMacro(false), _pTrayIco(NULL), _isUDDocked(false), _isRTL(false),
	_linkTriggered(true), _isDocModifing(false), _isHotspotDblClicked(false), _sysMenuEntering(false),
	_autoCompleteMain(&_mainEditView), _autoCompleteSub(&_subEditView), _smartHighlighter(&_findReplaceDlg),
	_nativeLangEncoding(CP_ACP)
{

	ZeroMemory(&_prevSelectedRange, sizeof(_prevSelectedRange));
	_winVersion = (NppParameters::getInstance())->getWinVersion();

	TiXmlDocumentA *nativeLangDocRootA = (NppParameters::getInstance())->getNativeLangA();

	if (nativeLangDocRootA)
	{

		_nativeLangA =  nativeLangDocRootA->FirstChild("NotepadPlus");
		if (_nativeLangA)
		{
			_nativeLangA = _nativeLangA->FirstChild("Native-Langue");
			if (_nativeLangA)
			{
				TiXmlElementA *element = _nativeLangA->ToElement();
				const char *rtl = element->Attribute("RTL");
				if (rtl)
					_isRTL = (strcmp(rtl, "yes") == 0);

				// get encoding
				TiXmlDeclarationA *declaration =  _nativeLangA->GetDocument()->FirstChild()->ToDeclaration();
				if (declaration)
				{
					const char * encodingStr = declaration->Encoding();
					_nativeLangEncoding = getCpFromStringValue(encodingStr);
				}
			}	
		}
	}
	else
		_nativeLangA = NULL;
	
	TiXmlDocument *toolIconsDocRoot = (NppParameters::getInstance())->getToolIcons();
	if (toolIconsDocRoot)
	{
		_toolIcons =  toolIconsDocRoot->FirstChild(TEXT("NotepadPlus"));
		if (_toolIcons)
		{
			if ((_toolIcons = _toolIcons->FirstChild(TEXT("ToolBarIcons"))))
			{
				if ((_toolIcons = _toolIcons->FirstChild(TEXT("Theme"))))
				{
					const TCHAR *themeDir = (_toolIcons->ToElement())->Attribute(TEXT("pathPrefix"));

					for (TiXmlNode *childNode = _toolIcons->FirstChildElement(TEXT("Icon"));
						 childNode ;
						 childNode = childNode->NextSibling(TEXT("Icon")))
					{
						int iIcon;
						const TCHAR *res = (childNode->ToElement())->Attribute(TEXT("id"), &iIcon);
						if (res)
						{
							TiXmlNode *grandChildNode = childNode->FirstChildElement(TEXT("normal"));
							if (grandChildNode)
							{
								TiXmlNode *valueNode = grandChildNode->FirstChild();
								//putain, enfin!!!
								if (valueNode)
								{
									generic_string locator = themeDir?themeDir:TEXT("");
									
									locator += valueNode->Value();
									_customIconVect.push_back(iconLocator(0, iIcon, locator));
								}
							}

							grandChildNode = childNode->FirstChildElement(TEXT("hover"));
							if (grandChildNode)
							{
								TiXmlNode *valueNode = grandChildNode->FirstChild();
								//putain, enfin!!!
								if (valueNode)
								{
									generic_string locator = themeDir?themeDir:TEXT("");
									
									locator += valueNode->Value();
									_customIconVect.push_back(iconLocator(1, iIcon, locator));
								}
							}

							grandChildNode = childNode->FirstChildElement(TEXT("disabled"));
							if (grandChildNode)
							{
								TiXmlNode *valueNode = grandChildNode->FirstChild();
								//putain, enfin!!!
								if (valueNode)
								{
									generic_string locator = themeDir?themeDir:TEXT("");
									
									locator += valueNode->Value();
									_customIconVect.push_back(iconLocator(2, iIcon, locator));
								}
							}
						}
					}
				}
			}
		}
	}
	else
		_toolIcons = NULL;
}

// ATTENTION : the order of the destruction is very important
// because if the parent's window hadle is destroyed before
// the destruction of its childrens' windows handle, 
// its childrens' windows handle will be destroyed automatically!
Notepad_plus::~Notepad_plus()
{
	(NppParameters::getInstance())->destroyInstance();
	MainFileManager->destroyInstance();
	(WcharMbcsConvertor::getInstance())->destroyInstance();
	if (_pTrayIco)
		delete _pTrayIco;
}

void Notepad_plus::init(HINSTANCE hInst, HWND parent, const TCHAR *cmdLine, CmdLineParams *cmdLineParams)
{	
	Window::init(hInst, parent);
	WNDCLASS nppClass;

	nppClass.style = CS_BYTEALIGNWINDOW | CS_DBLCLKS;//CS_HREDRAW | CS_VREDRAW;
	nppClass.lpfnWndProc = Notepad_plus_Proc;
	nppClass.cbClsExtra = 0;
	nppClass.cbWndExtra = 0;
	nppClass.hInstance = _hInst;
	nppClass.hIcon = ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON));
	nppClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	nppClass.hbrBackground = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
	nppClass.lpszMenuName = MAKEINTRESOURCE(IDR_M30_MENU);
	nppClass.lpszClassName = _className;

	if (!::RegisterClass(&nppClass))
	{
		systemMessage(TEXT("System Err"));
		throw int(98);
	}

	RECT workAreaRect;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

	NppParameters *pNppParams = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParams->getNppGUI();

	if (cmdLineParams->_isNoPlugin)
		_pluginsManager.disable();

	_hSelf = ::CreateWindowEx(
					WS_EX_ACCEPTFILES | (_isRTL?WS_EX_LAYOUTRTL:0),\
					_className,\
					TEXT("Notepad++"),\
					WS_OVERLAPPEDWINDOW	| WS_CLIPCHILDREN,\
					// CreateWindowEx bug : set all 0 to walk arround the pb
					0, 0, 0, 0,\
					_hParent,\
					NULL,\
					_hInst,\
					(LPVOID)this); // pass the ptr of this instantiated object
                                   // for retrive it in Notepad_plus_Proc from 
                                   // the CREATESTRUCT.lpCreateParams afterward.

	if (!_hSelf)
	{
		systemMessage(TEXT("System Err"));
		throw int(777);
	}

	gNppHWND = _hSelf;

	// In setting the startup window position, take into account that the last-saved
	// position might have assumed a second monitor that's no longer available.
	POINT newUpperLeft;
	newUpperLeft.x = nppGUI._appPos.left + workAreaRect.left;
	newUpperLeft.y = nppGUI._appPos.top + workAreaRect.top;

	// GetSystemMetrics does not support the multi-monitor values on Windows NT and Windows 95.
	if ((_winVersion != WV_95) && (_winVersion != WV_NT)) 
	{
		int margin = ::GetSystemMetrics(SM_CYSMCAPTION);
		if (newUpperLeft.x > ::GetSystemMetrics(SM_CXVIRTUALSCREEN)-margin)
			newUpperLeft.x = workAreaRect.right - nppGUI._appPos.right;
		if (newUpperLeft.x + nppGUI._appPos.right < ::GetSystemMetrics(SM_XVIRTUALSCREEN)+margin)
			newUpperLeft.x = workAreaRect.left;
		if (newUpperLeft.y > ::GetSystemMetrics(SM_CYVIRTUALSCREEN)-margin)
			newUpperLeft.y = workAreaRect.bottom - nppGUI._appPos.bottom;
		if (newUpperLeft.y + nppGUI._appPos.bottom < ::GetSystemMetrics(SM_YVIRTUALSCREEN)+margin)
			newUpperLeft.y = workAreaRect.top;
	}
	::MoveWindow(_hSelf, newUpperLeft.x, newUpperLeft.y, nppGUI._appPos.right, nppGUI._appPos.bottom, TRUE);

	::GetModuleFileName(NULL, _nppPath, MAX_PATH);

	if (nppGUI._tabStatus & TAB_MULTILINE)
		::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_DRAWTABBAR_MULTILINE, 0);

	if (!nppGUI._menuBarShow)
		::SetMenu(_hSelf, NULL);
	
	if (cmdLineParams->_isNoTab || (nppGUI._tabStatus & TAB_HIDE))
	{
		::SendMessage(_hSelf, NPPM_HIDETABBAR, 0, TRUE);
	}

	if (nppGUI._rememberLastSession && !cmdLineParams->_isNoSession)
	{
		loadLastSession();
	}

	::ShowWindow(_hSelf, nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);

    if (cmdLine)
    {
		loadCommandlineParams(cmdLine, cmdLineParams);
    }

#ifdef UNICODE
	LocalizationSwicher & localizationSwitcher = pNppParams->getLocalizationSwitcher();
	vector<wstring> fileNames;
	vector<wstring> patterns;
	patterns.push_back(TEXT("*.xml"));

	wchar_t tmp[MAX_PATH];
	lstrcpyW(tmp, _nppPath);
	::PathRemoveFileSpec(tmp);
	wstring localizationDir = tmp;
	
	localizationDir += TEXT("\\localization\\");
	getMatchedFileNames(localizationDir.c_str(), patterns, fileNames, false, false);
	for (size_t i = 0 ; i < fileNames.size() ; i++)
	{
		//wchar_t fullpath[MAX_PATH];
		//lstrcpyW(fn, localizationDir[i]);
		localizationSwitcher.addLanguageFromXml(fileNames[i].c_str());
	}
#endif

	// Notify plugins that Notepad++ is ready
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_READY;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = 0;
	_pluginsManager.notify(&scnN);

}



void Notepad_plus::killAllChildren() 
{
	_toolBar.destroy();
	_rebarTop.destroy();
	_rebarBottom.destroy();

    if (_pMainSplitter)
    {
        _pMainSplitter->destroy();
        delete _pMainSplitter;
    }

    _mainDocTab.destroy();
    _subDocTab.destroy();

	_mainEditView.destroy();
    _subEditView.destroy();
	_invisibleEditView.destroy();

    _subSplitter.destroy();
    _statusBar.destroy();

	_scintillaCtrls4Plugins.destroy();
	_dockingManager.destroy();
}


void Notepad_plus::destroy() 
{
	::DestroyWindow(_hSelf);
}

bool Notepad_plus::saveGUIParams()
{
	NppGUI & nppGUI = (NppGUI &)(NppParameters::getInstance())->getNppGUI();
	nppGUI._statusBarShow = _statusBar.isVisible();
	nppGUI._toolbarShow = _rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
	nppGUI._toolBarStatus = _toolBar.getState();

	nppGUI._tabStatus = (TabBarPlus::doDragNDropOrNot()?TAB_DRAWTOPBAR:0) | \
						(TabBarPlus::drawTopBar()?TAB_DRAGNDROP:0) | \
						(TabBarPlus::drawInactiveTab()?TAB_DRAWINACTIVETAB:0) | \
						(_toReduceTabBar?TAB_REDUCE:0) | \
						(TabBarPlus::drawTabCloseButton()?TAB_CLOSEBUTTON:0) | \
						(TabBarPlus::isDbClk2Close()?TAB_DBCLK2CLOSE:0) | \
						(TabBarPlus::isVertical() ? TAB_VERTICAL:0) | \
						(TabBarPlus::isMultiLine() ? TAB_MULTILINE:0) |\
						(nppGUI._tabStatus & TAB_HIDE);
	nppGUI._splitterPos = _subSplitter.isVertical()?POS_VERTICAL:POS_HORIZOTAL;
	UserDefineDialog *udd = _pEditView->getUserDefineDlg();
	bool b = udd->isDocked();
	nppGUI._userDefineDlgStatus = (b?UDD_DOCKED:0) | (udd->isVisible()?UDD_SHOW:0);
	
	// Save the position

	WINDOWPLACEMENT posInfo;

    posInfo.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(_hSelf, &posInfo);

	nppGUI._appPos.left = posInfo.rcNormalPosition.left;
	nppGUI._appPos.top = posInfo.rcNormalPosition.top;
	nppGUI._appPos.right = posInfo.rcNormalPosition.right - posInfo.rcNormalPosition.left;
	nppGUI._appPos.bottom = posInfo.rcNormalPosition.bottom - posInfo.rcNormalPosition.top;
	nppGUI._isMaximized = (IsZoomed(_hSelf) || (posInfo.flags & WPF_RESTORETOMAXIMIZED));

	saveDockingParams();

	return (NppParameters::getInstance())->writeGUIParams();
}

void Notepad_plus::saveDockingParams() 
{
	NppGUI & nppGUI = (NppGUI &)(NppParameters::getInstance())->getNppGUI();

	// Save the docking information
	nppGUI._dockingData._leftWidth		= _dockingManager.getDockedContSize(CONT_LEFT);
	nppGUI._dockingData._rightWidth		= _dockingManager.getDockedContSize(CONT_RIGHT); 
	nppGUI._dockingData._topHeight		= _dockingManager.getDockedContSize(CONT_TOP);	 
	nppGUI._dockingData._bottomHight	= _dockingManager.getDockedContSize(CONT_BOTTOM);

	// clear the conatainer tab information (active tab)
	nppGUI._dockingData._containerTabInfo.clear();

	// create a vector to save the current information
	vector<PlugingDlgDockingInfo>	vPluginDockInfo;
	vector<FloatingWindowInfo>		vFloatingWindowInfo;

	// save every container
	vector<DockingCont*> vCont = _dockingManager.getContainerInfo();

	for (size_t i = 0 ; i < vCont.size() ; i++)
	{
		// save at first the visible Tb's
		vector<tTbData *>	vDataVis	= vCont[i]->getDataOfVisTb();

		for (size_t j = 0 ; j < vDataVis.size() ; j++)
		{
			if (vDataVis[j]->pszName && vDataVis[j]->pszName[0])
			{
				PlugingDlgDockingInfo pddi(vDataVis[j]->pszModuleName, vDataVis[j]->dlgID, i, vDataVis[j]->iPrevCont, true);
				vPluginDockInfo.push_back(pddi);
			}
		}

		// save the hidden Tb's
		vector<tTbData *>	vDataAll	= vCont[i]->getDataOfAllTb();

		for (size_t j = 0 ; j < vDataAll.size() ; j++)
		{
			if ((vDataAll[j]->pszName && vDataAll[j]->pszName[0]) && (!vCont[i]->isTbVis(vDataAll[j])))
			{
				PlugingDlgDockingInfo pddi(vDataAll[j]->pszModuleName, vDataAll[j]->dlgID, i, vDataAll[j]->iPrevCont, false);
				vPluginDockInfo.push_back(pddi);
			}
		}

		// save the position, when container is a floated one
		if (i >= DOCKCONT_MAX)
		{
			RECT	rc;
			vCont[i]->getWindowRect(rc);
			FloatingWindowInfo fwi(i, rc.left, rc.top, rc.right, rc.bottom);
			vFloatingWindowInfo.push_back(fwi);
		}

		// save the active tab
		ContainerTabInfo act(i, vCont[i]->getActiveTb());
		nppGUI._dockingData._containerTabInfo.push_back(act);
	}

	// add the missing information and store it in nppGUI
	UCHAR floatContArray[50];
	memset(floatContArray, 0, 50);

	for (size_t i = 0 ; i < nppGUI._dockingData._pluginDockInfo.size() ; i++)
	{
		BOOL	isStored = FALSE;
		for (size_t j = 0; j < vPluginDockInfo.size(); j++)
		{
			if (nppGUI._dockingData._pluginDockInfo[i] == vPluginDockInfo[j])
			{
				isStored = TRUE;
				break;
			}
		}

		if (isStored == FALSE)
		{
			int floatCont	= 0;

			if (nppGUI._dockingData._pluginDockInfo[i]._currContainer >= DOCKCONT_MAX)
				floatCont = nppGUI._dockingData._pluginDockInfo[i]._currContainer;
			else
				floatCont = nppGUI._dockingData._pluginDockInfo[i]._prevContainer;

			if (floatContArray[floatCont] == 0)
			{
				RECT *pRc = nppGUI._dockingData.getFloatingRCFrom(floatCont);
				if (pRc)
					vFloatingWindowInfo.push_back(FloatingWindowInfo(floatCont, pRc->left, pRc->top, pRc->right, pRc->bottom));
				floatContArray[floatCont] = 1;
			}

			vPluginDockInfo.push_back(nppGUI._dockingData._pluginDockInfo[i]);
		}
	}

	nppGUI._dockingData._pluginDockInfo = vPluginDockInfo;
	nppGUI._dockingData._flaotingWindowInfo = vFloatingWindowInfo;
}

// return true if all the session files are loaded
// return false if one or more sessions files fail to load (and session is modify to remove invalid files)
bool Notepad_plus::loadSession(Session & session)
{
	bool allSessionFilesLoaded = true;
	BufferID lastOpened = BUFFER_INVALID;
	size_t i = 0;
	showView(MAIN_VIEW);
	switchEditViewTo(MAIN_VIEW);	//open files in main
	for ( ; i < session.nbMainFiles() ; )
	{
		const TCHAR *pFn = session._mainViewFiles[i]._fileName.c_str();
		if (isFileSession(pFn)) {
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions
		}
		if (PathFileExists(pFn)) {
			lastOpened = doOpen(pFn);
		} else {
			lastOpened = BUFFER_INVALID;
		}
		if (lastOpened != BUFFER_INVALID)
		{
			showView(MAIN_VIEW);
			const TCHAR *pLn = session._mainViewFiles[i]._langName.c_str();
			int id = getLangFromMenuName(pLn);
			LangType typeToSet = L_TXT;
			if (id != 0 && lstrcmp(pLn, TEXT("User Defined")) != 0)
				typeToSet = menuID2LangType(id);

			Buffer * buf = MainFileManager->getBufferByID(lastOpened);
			buf->setPosition(session._mainViewFiles[i], &_mainEditView);
			buf->setLangType(typeToSet, pLn);

			//Force in the document so we can add the markers
			//Dont use default methods because of performance
			Document prevDoc = _mainEditView.execute(SCI_GETDOCPOINTER);
			_mainEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			for (size_t j = 0 ; j < session._mainViewFiles[i].marks.size() ; j++) {
				_mainEditView.execute(SCI_MARKERADD, session._mainViewFiles[i].marks[j], MARK_BOOKMARK);
			}
			_mainEditView.execute(SCI_SETDOCPOINTER, 0, prevDoc);
			i++;
		}
		else
		{
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			allSessionFilesLoaded = false;
		}
	}

	size_t k = 0;
	showView(SUB_VIEW);
	switchEditViewTo(SUB_VIEW);	//open files in sub
	for ( ; k < session.nbSubFiles() ; )
	{
		const TCHAR *pFn = session._subViewFiles[k]._fileName.c_str();
		if (isFileSession(pFn)) {
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			continue;	//skip session files, not supporting recursive sessions
		}
		if (PathFileExists(pFn)) {
			lastOpened = doOpen(pFn);
			//check if already open in main. If so, clone
			if (_mainDocTab.getIndexByBuffer(lastOpened) != -1) {
				loadBufferIntoView(lastOpened, SUB_VIEW);
			}
		} else {
			lastOpened = BUFFER_INVALID;
		}
		if (lastOpened != BUFFER_INVALID)
		{
			showView(SUB_VIEW);
			if (canHideView(MAIN_VIEW))
				hideView(MAIN_VIEW);
			const TCHAR *pLn = session._subViewFiles[k]._langName.c_str();
			int id = getLangFromMenuName(pLn);
			LangType typeToSet = L_TXT;
			if (id != 0)
				typeToSet = menuID2LangType(id);

			Buffer * buf = MainFileManager->getBufferByID(lastOpened);
			buf->setPosition(session._subViewFiles[k], &_subEditView);
			if (typeToSet == L_USER) {
				if (!lstrcmp(pLn, TEXT("User Defined"))) {
					pLn = TEXT("");	//default user defined
				}
			}
			buf->setLangType(typeToSet, pLn);
			
			//Force in the document so we can add the markers
			//Dont use default methods because of performance
			Document prevDoc = _subEditView.execute(SCI_GETDOCPOINTER);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			for (size_t j = 0 ; j < session._subViewFiles[k].marks.size() ; j++) {
				_subEditView.execute(SCI_MARKERADD, session._subViewFiles[k].marks[j], MARK_BOOKMARK);
			}
			_subEditView.execute(SCI_SETDOCPOINTER, 0, prevDoc);

			k++;
		}
		else
		{
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			allSessionFilesLoaded = false;
		}
	}

	_mainEditView.restoreCurrentPos();
	_subEditView.restoreCurrentPos();

	if (session._activeMainIndex < (size_t)_mainDocTab.nbItem())//session.nbMainFiles())
		activateBuffer(_mainDocTab.getBufferByIndex(session._activeMainIndex), MAIN_VIEW);

	if (session._activeSubIndex < (size_t)_subDocTab.nbItem())//session.nbSubFiles())
		activateBuffer(_subDocTab.getBufferByIndex(session._activeSubIndex), SUB_VIEW);

	if ((session.nbSubFiles() > 0) && (session._activeView == MAIN_VIEW || session._activeView == SUB_VIEW))
		switchEditViewTo(session._activeView);
	else
		switchEditViewTo(MAIN_VIEW);

	if (canHideView(otherView()))
		hideView(otherView());
	else if (canHideView(currentView()))
		hideView(currentView());
	return allSessionFilesLoaded;
}

BufferID Notepad_plus::doOpen(const TCHAR *fileName, bool isReadOnly)
{	
	TCHAR longFileName[MAX_PATH];
	::GetFullPathName(fileName, MAX_PATH, longFileName, NULL);
	::GetLongPathName(longFileName, longFileName, MAX_PATH);

	_lastRecentFileList.remove(longFileName);

	BufferID test = MainFileManager->getBufferFromName(longFileName);
	if (test != BUFFER_INVALID)
	{
		//switchToFile(test);
		//Dont switch, not responsibility of doOpen, but of caller
		if (_pTrayIco)
		{
			if (_pTrayIco->isInTray())
			{
				::ShowWindow(_hSelf, SW_SHOW);
				_pTrayIco->doTrayIcon(REMOVE);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
		}
		return test;
	}

	if (isFileSession(longFileName) && PathFileExists(longFileName)) {
		fileLoadSession(longFileName);
		return BUFFER_INVALID;
	}
	
	if (!PathFileExists(longFileName))
	{
		TCHAR str2display[MAX_PATH*2];
		TCHAR longFileDir[MAX_PATH];

		lstrcpy(longFileDir, longFileName);
		PathRemoveFileSpec(longFileDir);

		if (PathFileExists(longFileDir))
		{
			wsprintf(str2display, TEXT("%s doesn't exist. Create it?"), longFileName);

			if (::MessageBox(_hSelf, str2display, TEXT("Create new file"), MB_YESNO) == IDYES)
			{
				bool res = MainFileManager->createEmptyFile(longFileName);
				if (!res) {
					wsprintf(str2display, TEXT("Cannot create the file \"%s\""), longFileName);
					::MessageBox(_hSelf, str2display, TEXT("Create new file"), MB_OK);
					return BUFFER_INVALID;
				}
			}
			else
			{
				return BUFFER_INVALID;
			}
		}
		else
		{
			return BUFFER_INVALID;
		}
	}


	BufferID buffer = MainFileManager->loadFile(longFileName);
	if (buffer != BUFFER_INVALID)
	{
		Buffer * buf = MainFileManager->getBufferByID(buffer);
		// if file is read only, we set the view read only
		if (isReadOnly)
			buf->setUserReadOnly(true);

		// Notify plugins that current file is just opened
		SCNotification scnN;
		scnN.nmhdr.code = NPPN_FILEBEFOREOPEN;
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = (uptr_t)buffer;
		_pluginsManager.notify(&scnN);

		loadBufferIntoView(buffer, currentView());

		if (_pTrayIco)
		{
			if (_pTrayIco->isInTray())
			{
				::ShowWindow(_hSelf, SW_SHOW);
				_pTrayIco->doTrayIcon(REMOVE);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
		}
		PathRemoveFileSpec(longFileName);
		_linkTriggered = true;
		_isDocModifing = false;
		
		// Notify plugins that current file is just opened
		scnN.nmhdr.code = NPPN_FILEOPENED;
		_pluginsManager.notify(&scnN);

		return buffer;
	}
	else
	{
		TCHAR msg[MAX_PATH + 100];
		lstrcpy(msg, TEXT("Can not open file \""));
		//lstrcat(msg, fullPath);
		lstrcat(msg, longFileName);
		lstrcat(msg, TEXT("\"."));
		::MessageBox(_hSelf, msg, TEXT("ERR"), MB_OK);
		return BUFFER_INVALID;
	}
}
bool Notepad_plus::doReload(BufferID id, bool alert)
{

	/*
	//No activation when reloading, defer untill document is actually visible
	if (alert) {
		switchToFile(id);
	}
	*/
	if (alert)
	{
		if (::MessageBox(_hSelf, TEXT("Are you sure you want to reload the current file and lose the changes made in Notepad++?"), TEXT("Reload"), MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL) != IDYES)
			return false;
	}

	//In order to prevent Scintilla from restyling the entire document,
	//an empty Document is inserted during reload if needed.
	bool mainVisisble = (_mainEditView.getCurrentBufferID() == id);
	bool subVisisble = (_subEditView.getCurrentBufferID() == id);
	if (mainVisisble) {
		_mainEditView.saveCurrentPos();
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, 0);
	}
	if (subVisisble) {
		_subEditView.saveCurrentPos();
		_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
	}

	if (!mainVisisble && !subVisisble) {
		return MainFileManager->reloadBufferDeferred(id);
	}

	bool res = MainFileManager->reloadBuffer(id);
	Buffer * pBuf = MainFileManager->getBufferByID(id);
	if (mainVisisble) {
		_mainEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_mainEditView.restoreCurrentPos();
	}
	if (subVisisble) {
		_subEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
		_subEditView.restoreCurrentPos();
	}
	return res;
}

bool Notepad_plus::doSave(BufferID id, const TCHAR * filename, bool isCopy)
{
	SCNotification scnN;
	// Notify plugins that current file is about to be saved
	if (!isCopy)
	{
		
		scnN.nmhdr.code = NPPN_FILEBEFORESAVE;
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = (uptr_t)id;
		_pluginsManager.notify(&scnN);
	}

	bool res = MainFileManager->saveBuffer(id, filename, isCopy);

	if (!isCopy)
	{
		scnN.nmhdr.code = NPPN_FILESAVED;
		_pluginsManager.notify(&scnN);
	}

	if (!res)
		::MessageBox(_hSelf, TEXT("Please check whether if this file is opened in another program"), TEXT("Save failed"), MB_OK);
	return res;
}

void Notepad_plus::doClose(BufferID id, int whichOne) {
	Buffer * buf = MainFileManager->getBufferByID(id);

	// Notify plugins that current file is about to be closed
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORECLOSE;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = (uptr_t)id;
	_pluginsManager.notify(&scnN);

	//add to recent files if its an existing file
	if (!buf->isUntitled() && PathFileExists(buf->getFullPathName()))
	{
		_lastRecentFileList.add(buf->getFullPathName());
	}

	int nrDocs = whichOne==MAIN_VIEW?(_mainDocTab.nbItem()):(_subDocTab.nbItem());

	//Do all the works
	removeBufferFromView(id, whichOne);
	if (nrDocs == 1 && canHideView(whichOne))
	{	//close the view if both visible
		hideView(whichOne);
	}

	// Notify plugins that current file is closed
	scnN.nmhdr.code = NPPN_FILECLOSED;
	_pluginsManager.notify(&scnN);

	return;
}
void Notepad_plus::fileNew()
{
	BufferID newBufID = MainFileManager->newEmptyDocument();
	loadBufferIntoView(newBufID, currentView(), true);	//true, because we want multiple new files if possible
	activateBuffer(newBufID, currentView());
}

bool Notepad_plus::fileReload()
{
	BufferID buf = _pEditView->getCurrentBufferID();
	return doReload(buf, buf->isDirty());
}

generic_string exts2Filters(generic_string exts) {
	const TCHAR *extStr = exts.c_str();
	TCHAR aExt[MAX_PATH];
	generic_string filters(TEXT(""));

	int j = 0;
	bool stop = false;
	for (size_t i = 0 ; i < exts.length() ; i++)
	{
		if (extStr[i] == ' ')
		{
			if (!stop)
			{
				aExt[j] = '\0';
				stop = true;

				if (aExt[0])
				{
					filters += TEXT("*.");
					filters += aExt;
					filters += TEXT(";");
				}
				j = 0;
			}
		}
		else
		{
			aExt[j] = extStr[i];
			stop = false;
			j++;
		}
	}

	if (j > 0)
	{
		aExt[j] = '\0';
		if (aExt[0])
		{
			filters += TEXT("*.");
			filters += aExt;
			filters += TEXT(";");
		}
	}

	// remove the last ';'
    filters = filters.substr(0, filters.length()-1);
	return filters;
};

void Notepad_plus::setFileOpenSaveDlgFilters(FileDialog & fDlg)
{
	NppParameters *pNppParam = NppParameters::getInstance();
	NppGUI & nppGUI = (NppGUI & )pNppParam->getNppGUI();

	int i = 0;
	Lang *l = NppParameters::getInstance()->getLangFromIndex(i++);
	LangType curl = _pEditView->getCurrentBuffer()->getLangType();

	while (l)
	{
		LangType lid = l->getLangID();

		bool inExcludedList = false;
		
		for (size_t j = 0 ; j < nppGUI._excludedLangList.size() ; j++)
		{
			if (lid == nppGUI._excludedLangList[j]._langType)
			{
				inExcludedList = true;
				break;
			}
		}

		if (!inExcludedList)
		{
			const TCHAR *defList = l->getDefaultExtList();
			const TCHAR *userList = NULL;

			LexerStylerArray &lsa = (NppParameters::getInstance())->getLStylerArray();
			const TCHAR *lName = l->getLangName();
			LexerStyler *pLS = lsa.getLexerStylerByName(lName);
			
			if (pLS)
				userList = pLS->getLexerUserExt();

			std::generic_string list(TEXT(""));
			if (defList)
				list += defList;
			if (userList)
			{
				list += TEXT(" ");
				list += userList;
			}
			
			generic_string stringFilters = exts2Filters(list);
			const TCHAR *filters = stringFilters.c_str();
			if (filters[0])
			{
				int nbExt = fDlg.setExtsFilter(getLangDesc(lid, true).c_str(), filters);
			}
		}
		l = (NppParameters::getInstance())->getLangFromIndex(i++);
	}
}

void Notepad_plus::fileOpen()
{
    FileDialog fDlg(_hSelf, _hInst);
	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	
	setFileOpenSaveDlgFilters(fDlg);

	BufferID lastOpened = BUFFER_INVALID;
	if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
	{
		size_t sz = pfns->size();
		for (size_t i = 0 ; i < sz ; i++) {
			BufferID test = doOpen(pfns->at(i).c_str(), fDlg.isReadOnly());
			if (test != BUFFER_INVALID)
				lastOpened = test;
		}
	}
	if (lastOpened != BUFFER_INVALID) {
		switchToFile(lastOpened);
	}
}

bool Notepad_plus::isFileSession(const TCHAR * filename) {
	// if file2open matches the ext of user defined session file ext, then it'll be opened as a session
	const TCHAR *definedSessionExt = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();
	if (*definedSessionExt != '\0')
	{
		TCHAR fncp[MAX_PATH];
		lstrcpy(fncp, filename);
		TCHAR *pExt = PathFindExtension(fncp);
		generic_string usrSessionExt = TEXT("");
		if (*definedSessionExt != '.')
		{
			usrSessionExt += TEXT(".");
		}
		usrSessionExt += definedSessionExt;

		if (!generic_stricmp(pExt, usrSessionExt.c_str()))
		{
			return true;
		}
	}
	return false;
}

bool Notepad_plus::fileSave(BufferID id)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	if (!buf->getFileReadOnly() && buf->isDirty())	//cannot save if readonly
	{
		const TCHAR *fn = buf->getFullPathName();
		if (buf->isUntitled())
		{
			return fileSaveAs(id);
		}
		else
		{
			const NppGUI & nppgui = (NppParameters::getInstance())->getNppGUI();
			BackupFeature backup = nppgui._backup;
			if (backup == bak_simple)
			{
				//copy fn to fn.backup
				generic_string fn_bak(fn);
				if ((nppgui._useDir) && (nppgui._backupDir[0] != '\0'))
				{
					TCHAR path[MAX_PATH];
					TCHAR *name;

					lstrcpy(path, fn);
					name = ::PathFindFileName(path);
					fn_bak = nppgui._backupDir;
					fn_bak += TEXT("\\");
					fn_bak += name;
				}
				else
				{
					fn_bak = fn;
				}
				fn_bak += TEXT(".bak");
				::CopyFile(fn, fn_bak.c_str(), FALSE);
			}
			else if (backup == bak_verbose)
			{
				TCHAR path[MAX_PATH];
				TCHAR *name;
				generic_string fn_dateTime_bak;
				
				lstrcpy(path, fn);

				name = ::PathFindFileName(path);
				::PathRemoveFileSpec(path);
				
				if ((nppgui._useDir) && (nppgui._backupDir[0] != '\0'))
				{
					fn_dateTime_bak = nppgui._backupDir;
					fn_dateTime_bak += TEXT("\\");
				}
				else
				{
					const TCHAR *bakDir = TEXT("nppBackup");
					fn_dateTime_bak = path;
					fn_dateTime_bak += TEXT("\\");
					fn_dateTime_bak += bakDir;
					fn_dateTime_bak += TEXT("\\");

					if (!::PathFileExists(fn_dateTime_bak.c_str()))
					{
						::CreateDirectory(fn_dateTime_bak.c_str(), NULL);
					}
				}

				fn_dateTime_bak += name;

				const int temBufLen = 32;
				TCHAR tmpbuf[temBufLen];
				time_t ltime = time(0);
				struct tm *today;

				today = localtime(&ltime);
				generic_strftime(tmpbuf, temBufLen, TEXT("%Y-%m-%d_%H%M%S"), today);

				fn_dateTime_bak += TEXT(".");
				fn_dateTime_bak += tmpbuf;
				fn_dateTime_bak += TEXT(".bak");

				::CopyFile(fn, fn_dateTime_bak.c_str(), FALSE);
			}
			return doSave(bufferID, buf->getFullPathName(), false);
		}
	}
	return false;
}

bool Notepad_plus::fileSaveAll() {
	if (viewVisible(MAIN_VIEW)) {
		for(int i = 0; i < _mainDocTab.nbItem(); i++) {
			BufferID idToSave = _mainDocTab.getBufferByIndex(i);
			fileSave(idToSave);
		}
	}

	if (viewVisible(SUB_VIEW)) {
		for(int i = 0; i < _subDocTab.nbItem(); i++) {
			BufferID idToSave = _subDocTab.getBufferByIndex(i);
			fileSave(idToSave);
		}
	}
	return true;
}

bool Notepad_plus::fileSaveAs(BufferID id, bool isSaveCopy)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	FileDialog fDlg(_hSelf, _hInst);

    fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	setFileOpenSaveDlgFilters(fDlg);
			
	fDlg.setDefFileName(buf->getFileName());
	TCHAR *pfn = fDlg.doSaveDlg();

	if (pfn)
	{
		BufferID other = _pNonDocTab->findBufferByName(pfn);
		if (other == BUFFER_INVALID)	//can save, other view doesnt contain buffer
		{
			bool res = doSave(bufferID, pfn, isSaveCopy);
			//buf->setNeedsLexing(true);	//commented to fix wrapping being removed after save as (due to SCI_CLEARSTYLE or something, seems to be Scintilla bug)
			//Changing lexer after save seems to work properly
			return res;
		}
		else		//cannot save, other view has buffer already open, activate it
		{
			::MessageBox(_hSelf, TEXT("The file is already opened in the Notepad++."), TEXT("ERROR"), MB_OK | MB_ICONSTOP);
			switchToFile(other);
			return false;
		}
	}
	else // cancel button is pressed
    {
        checkModifiedDocument();
		return false;
    }
}

bool Notepad_plus::fileRename(BufferID id, int curView)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	FileDialog fDlg(_hSelf, _hInst);

    fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	setFileOpenSaveDlgFilters(fDlg);
			
	fDlg.setDefFileName(buf->getFileName());
	TCHAR *pfn = fDlg.doSaveDlg();

	if (pfn)
	{
		MainFileManager->moveFile(bufferID, pfn);
	}
	return false;
}
/*
bool Notepad_plus::fileDelete(BufferID id, int curView)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	
	Buffer * buf = MainFileManager->getBufferByID(bufferID);
	const TCHAR *fileNamePath = buf->getFullPathName();

	if (PathFileExists(fileNamePath))
		return false;

	int res = doDeleteOrNot(fileNamePath);
	if (res == IDYES && doDelete(fileNamePath))
	{
		doClose(bufferID, currentView());
		return true;
	}
	return false;
}
*/

bool Notepad_plus::fileDelete(BufferID id, int curView)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	
	Buffer * buf = MainFileManager->getBufferByID(bufferID);
	const TCHAR *fileNamePath = buf->getFullPathName();

	if (doDeleteOrNot(fileNamePath) == IDYES)
	{
		if (!MainFileManager->deleteFile(bufferID))
		{
			::MessageBox(_hSelf, TEXT("Delete File failed"), TEXT("Delete File"), MB_OK);
			return false;
		}
		doClose(bufferID, MAIN_VIEW);
		doClose(bufferID, SUB_VIEW);
		return true;
	}
	return false;
}

bool Notepad_plus::fileClose(BufferID id, int curView)
{
	BufferID bufferID = id;
	if (id == BUFFER_INVALID)
		bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);

	int res;

	//process the fileNamePath into LRF
	const TCHAR *fileNamePath = buf->getFullPathName();

	if (buf->isDirty())
	{
		res = doSaveOrNot(fileNamePath);
		if (res == IDYES)
		{
			if (!fileSave(id)) // the cancel button of savedialog is pressed, aborts closing
				return false;
		}
		else if (res == IDCANCEL)
		{
			return false;	//cancel aborts closing
		}
		else
		{
			// else IDNO we continue
		}
	}

	int viewToClose = currentView();
	if (curView != -1)
		viewToClose = curView;
	//first check amount of documents, we dont want the view to hide if we closed a secondary doc with primary being empty
	//int nrDocs = _pDocTab->nbItem();
	doClose(bufferID, viewToClose);
	//if (nrDocs == 1 && canHideView(currentView())) {	//close the view if both visible
	//	hideView(viewToClose);
	//}

	return true;
}

bool Notepad_plus::fileCloseAll()
{
	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for(int i = 0; i < _mainDocTab.nbItem(); i++) {
		BufferID id = _mainDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isDirty()) {
			int res = doSaveOrNot(buf->getFullPathName());
			if (res == IDYES) {
				if (!fileSave(id))
					return false;	//abort entire procedure
			} else if (res == IDCANCEL) {
					return false;
				//otherwise continue (IDNO)
			}
		}
	}
	for(int i = 0; i < _subDocTab.nbItem(); i++) {
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isDirty()) {
			int res = doSaveOrNot(buf->getFullPathName());
			if (res == IDYES) {
				if (!fileSave(id))
					return false;	//abort entire procedure
			} else if (res == IDCANCEL) {
					return false;
				//otherwise continue (IDNO)
			}
		}
	}

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {	//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		for(int i = _pNonDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
			doClose(_pNonDocTab->getBufferByIndex(i), otherView());
		}
		//hideView(otherView());
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for(int i = _pDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
		doClose(_pDocTab->getBufferByIndex(i), currentView());
	}
	return true;
}

bool Notepad_plus::fileCloseAllButCurrent()
{
	BufferID current = _pEditView->getCurrentBufferID();
	int active = _pDocTab->getCurrentTabIndex();
	//closes all documents, makes the current view the only one visible

	//first check if we need to save any file
	for(int i = 0; i < _mainDocTab.nbItem(); i++) {
		BufferID id = _mainDocTab.getBufferByIndex(i);
		if (id == current)
			continue;
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (buf->isDirty()) {
			int res = doSaveOrNot(buf->getFullPathName());
			if (res == IDYES) {
				if (!fileSave(id))
					return false;	//abort entire procedure
			} else if (res == IDCANCEL) {
					return false;
				//otherwise continue (IDNO)
			}
		}
	}
	for(int i = 0; i < _subDocTab.nbItem(); i++) {
		BufferID id = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(id);
		if (id == current)
			continue;
		if (buf->isDirty()) {
			int res = doSaveOrNot(buf->getFullPathName());
			if (res == IDYES) {
				if (!fileSave(id))
					return false;	//abort entire procedure
			} else if (res == IDCANCEL) {
					return false;
				//otherwise continue (IDNO)
			}
		}
	}

	//Then start closing, inactive view first so the active is left open
    if (bothActive())
    {	//first close all docs in non-current view, which gets closed automatically
		//Set active tab to the last one closed.
		activateBuffer(_pNonDocTab->getBufferByIndex(0), otherView());
		for(int i = _pNonDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
			doClose(_pNonDocTab->getBufferByIndex(i), otherView());
		}
		//hideView(otherView());
    }

	activateBuffer(_pDocTab->getBufferByIndex(0), currentView());
	for(int i = _pDocTab->nbItem() - 1; i >= 0; i--) {	//close all from right to left
		if (i == active) {	//dont close active index
			continue;
		}
		doClose(_pDocTab->getBufferByIndex(i), currentView());
	}
	return true;
}

bool Notepad_plus::replaceAllFiles() {

	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);
	Buffer * oldBuf = _invisibleEditView.getCurrentBuffer();	//for manually setting the buffer, so notifications can be handled properly

	Buffer * pBuf = NULL;

	int nbTotal = 0;
	const bool isEntireDoc = true;

    if (_mainWindowStatus & WindowMainActive)
    {
		for (int i = 0 ; i < _mainDocTab.nbItem() ; i++)
	    {
			pBuf = MainFileManager->getBufferByID(_mainDocTab.getBufferByIndex(i));
			if (pBuf->isReadOnly())
				continue;
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? 0 : SC_CP_UTF8);
			_invisibleEditView._currentBuffer = pBuf;
		    _invisibleEditView.execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(ProcessReplaceAll, NULL, NULL, isEntireDoc, NULL);
			_invisibleEditView.execute(SCI_ENDUNDOACTION);
		}
	}
	
	if (_mainWindowStatus & WindowSubActive)
    {
		for (int i = 0 ; i < _subDocTab.nbItem() ; i++)
	    {
			pBuf = MainFileManager->getBufferByID(_subDocTab.getBufferByIndex(i));
			if (pBuf->isReadOnly())
				continue;
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? 0 : SC_CP_UTF8);
			_invisibleEditView._currentBuffer = pBuf;
		    _invisibleEditView.execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(ProcessReplaceAll, NULL, NULL, isEntireDoc, NULL);
			_invisibleEditView.execute(SCI_ENDUNDOACTION);
		}
	}

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_invisibleEditView._currentBuffer = oldBuf;
	_pEditView = pOldView;

	TCHAR result[64];
	if (nbTotal < 0)
		lstrcpy(result, TEXT("The regular expression to search is formed badly"));
	else
		wsprintf(result, TEXT("%d occurrences were replaced."), nbTotal);

	::MessageBox(_hSelf, result, TEXT(""), MB_OK);

	return true;
}

bool Notepad_plus::matchInList(const TCHAR *fileName, const vector<generic_string> & patterns)
{
	for (size_t i = 0 ; i < patterns.size() ; i++)
	{
		if (PathMatchSpec(fileName, patterns[i].c_str()))
			return true;
	}
	return false;
}

void Notepad_plus::saveFindHistory()
{
	_findReplaceDlg.saveFindHistory();
	(NppParameters::getInstance())->writeFindHistory();
}

void Notepad_plus::saveUserDefineLangs() 
{
	if (ScintillaEditView::getUserDefineDlg()->isDirty())
		(NppParameters::getInstance())->writeUserDefinedLang();
}

void Notepad_plus::saveShortcuts()
{
	NppParameters::getInstance()->writeShortcuts();
}

void Notepad_plus::saveSession(const Session & session)
{
	(NppParameters::getInstance())->writeSession(session);
}

void Notepad_plus::doTrimTrailing() 
{
	_pEditView->execute(SCI_BEGINUNDOACTION);
	int nbLines = _pEditView->execute(SCI_GETLINECOUNT);
	for (int line = 0 ; line < nbLines ; line++)
	{
		int lineStart = _pEditView->execute(SCI_POSITIONFROMLINE,line);
		int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION,line);
		int i = lineEnd - 1;
		char c = (char)_pEditView->execute(SCI_GETCHARAT,i);

		for ( ; (i >= lineStart) && (c == ' ') || (c == '\t') ; c = (char)_pEditView->execute(SCI_GETCHARAT,i))
			i--;

		if (i < (lineEnd - 1))
			_pEditView->replaceTarget(TEXT(""), i + 1, lineEnd);
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::loadLastSession() 
{
	Session lastSession = (NppParameters::getInstance())->getSession();
	loadSession(lastSession);
}

void Notepad_plus::getMatchedFileNames(const TCHAR *dir, const vector<generic_string> & patterns, vector<generic_string> & fileNames, bool isRecursive, bool isInHiddenDir)
{
	generic_string dirFilter(dir);
	dirFilter += TEXT("*.*");
	WIN32_FIND_DATA foundData;

	HANDLE hFile = ::FindFirstFile(dirFilter.c_str(), &foundData);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		
		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				// branles rien
			}
			else if (isRecursive)
			{
				if ((lstrcmp(foundData.cFileName, TEXT("."))) && (lstrcmp(foundData.cFileName, TEXT(".."))))
				{
					generic_string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");
					getMatchedFileNames(pathDir.c_str(), patterns, fileNames, isRecursive, isInHiddenDir);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				generic_string pathFile(dir);
				pathFile += foundData.cFileName;
				fileNames.push_back(pathFile.c_str());
			}
		}
	}
	while (::FindNextFile(hFile, &foundData))
	{
		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!isInHiddenDir && (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{
				// branles rien
			}
			else if (isRecursive)
			{
				if ((lstrcmp(foundData.cFileName, TEXT("."))) && (lstrcmp(foundData.cFileName, TEXT(".."))))
				{
					generic_string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += TEXT("\\");
					getMatchedFileNames(pathDir.c_str(), patterns, fileNames, isRecursive, isInHiddenDir);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				generic_string pathFile(dir);
				pathFile += foundData.cFileName;
				fileNames.push_back(pathFile.c_str());
			}
		}
	}
	::FindClose(hFile);
}

DWORD WINAPI AsyncCancelFindInFiles(LPVOID NppHWND) 
{
	MessageBox((HWND) NULL, TEXT("Searching...\nPress Enter to Cancel"), TEXT("Find In Files"), MB_OK);
	PostMessage((HWND) NppHWND, NPPM_INTERNAL_CANCEL_FIND_IN_FILES, 0, 0);
	return 0;
}

bool Notepad_plus::findInFiles()
{
	bool isRecursive = _findReplaceDlg.isRecursive();
	bool isInHiddenDir = _findReplaceDlg.isInHiddenDir();
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	if (!_findReplaceDlg.isFinderEmpty())
		_findReplaceDlg.clearFinder();

	const TCHAR *dir2Search = _findReplaceDlg.getDir2Search();

	if (!dir2Search[0] || !::PathFileExists(dir2Search))
	{
		return false;
	}

	HANDLE CancelThreadHandle = ::CreateThread(NULL, 0, AsyncCancelFindInFiles, _hSelf, 0, NULL);

	vector<generic_string> patterns2Match;
	if (_findReplaceDlg.getFilters() == TEXT(""))
		_findReplaceDlg.setFindInFilesDirFilter(NULL, TEXT("*.*"));
	_findReplaceDlg.getPatterns(patterns2Match);
	vector<generic_string> fileNames;
		
	_findReplaceDlg.putFindResultStr(TEXT("Scanning files to search..."));
	_findReplaceDlg.refresh();

	getMatchedFileNames(dir2Search, patterns2Match, fileNames, isRecursive, isInHiddenDir);

	TCHAR msg[128];
	wsprintf(msg, TEXT("Found %d matching files"), fileNames.size());
	_findReplaceDlg.putFindResultStr((const TCHAR*)msg);
	_findReplaceDlg.refresh();

	bool dontClose = false;
	for (size_t i = 0 ; i < fileNames.size() ; i++)
	{
		MSG msg;
		if (PeekMessage(&msg, _hSelf, NPPM_INTERNAL_CANCEL_FIND_IN_FILES, NPPM_INTERNAL_CANCEL_FIND_IN_FILES, PM_REMOVE)) break;

		BufferID id = MainFileManager->getBufferFromName(fileNames.at(i).c_str());
		if (id != BUFFER_INVALID) 
		{
			dontClose = true;
		} 
		else 
		{
			id = MainFileManager->loadFile(fileNames.at(i).c_str());
			dontClose = false;
		}
		
		if (id != BUFFER_INVALID) 
		{
			Buffer * pBuf = MainFileManager->getBufferByID(id);
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? 0 : SC_CP_UTF8);
			
			generic_string str = TEXT("File: ");
			str += fileNames.at(i);
			_findReplaceDlg.putFindResultStr(str.c_str());

			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, NULL, NULL, true, fileNames.at(i).c_str());
			if (!dontClose)
				MainFileManager->closeBuffer(id, _pEditView);
		}
	}

	TerminateThread(CancelThreadHandle, 0);

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;
	
	wsprintf(msg, TEXT("%d hits"), nbTotal);
	_findReplaceDlg.putFindResultStr((const TCHAR *)&msg);
	_findReplaceDlg.refresh();

	return true;
}

bool Notepad_plus::findInOpenedFiles()
{
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;
	_pEditView = &_invisibleEditView;
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	Buffer * pBuf = NULL;

	const bool isEntireDoc = true;

	if (!_findReplaceDlg.isFinderEmpty())
		_findReplaceDlg.clearFinder();
	
	_findReplaceDlg.setSearchWord2Finder();
	
    if (_mainWindowStatus & WindowMainActive)
    {
		for (int i = 0 ; i < _mainDocTab.nbItem() ; i++)
	    {
			pBuf = MainFileManager->getBufferByID(_mainDocTab.getBufferByIndex(i));
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? 0 : SC_CP_UTF8);
			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, NULL, NULL, isEntireDoc, pBuf->getFullPathName());
	    }
    }
    
    if (_mainWindowStatus & WindowSubActive)
    {
		for (int i = 0 ; i < _subDocTab.nbItem() ; i++)
	    {
			pBuf = MainFileManager->getBufferByID(_subDocTab.getBufferByIndex(i));
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, pBuf->getDocument());
			_invisibleEditView.execute(SCI_SETCODEPAGE, pBuf->getUnicodeMode() == uni8Bit ? 0 : SC_CP_UTF8);
			nbTotal += _findReplaceDlg.processAll(ProcessFindAll, NULL, NULL, isEntireDoc, pBuf->getFullPathName());
	    }
    }

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
	_pEditView = pOldView;

	_findReplaceDlg.putFindResult(nbTotal);
	return true;
}

void Notepad_plus::filePrint(bool showDialog)
{
	Printer printer;

	int startPos = int(_pEditView->execute(SCI_GETSELECTIONSTART));
	int endPos = int(_pEditView->execute(SCI_GETSELECTIONEND));
	
	printer.init(_hInst, _hSelf, _pEditView, showDialog, startPos, endPos);
	printer.doPrint();
}

void Notepad_plus::enableCommand(int cmdID, bool doEnable, int which) const
{
	if (which & MENU)
	{
		enableMenu(cmdID, doEnable);
	}
	if (which & TOOLBAR)
	{
		_toolBar.enable(cmdID, doEnable);
	}
}

void Notepad_plus::checkClipboard() 
{
	bool hasSelection = _pEditView->execute(SCI_GETSELECTIONSTART) != _pEditView->execute(SCI_GETSELECTIONEND);
	bool canPaste = (_pEditView->execute(SCI_CANPASTE) != 0);
	enableCommand(IDM_EDIT_CUT, hasSelection, MENU | TOOLBAR); 
	enableCommand(IDM_EDIT_COPY, hasSelection, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_PASTE, canPaste, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_DELETE, hasSelection, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_UPPERCASE, hasSelection, MENU);
	enableCommand(IDM_EDIT_LOWERCASE, hasSelection, MENU);
	enableCommand(IDM_EDIT_BLOCK_COMMENT, hasSelection, MENU);
	enableCommand(IDM_EDIT_BLOCK_COMMENT_SET, hasSelection, MENU);
	enableCommand(IDM_EDIT_BLOCK_UNCOMMENT, hasSelection, MENU);
	enableCommand(IDM_EDIT_STREAM_COMMENT, hasSelection, MENU);
}

void Notepad_plus::checkDocState()
{
	Buffer * curBuf = _pEditView->getCurrentBuffer();

	bool isCurrentDirty = curBuf->isDirty();
	bool isSeveralDirty = isCurrentDirty;
	bool isFileExisting = PathFileExists(curBuf->getFullPathName()) != FALSE;
	if (!isCurrentDirty)
	{
		for(int i = 0; i < MainFileManager->getNrBuffers(); i++)
		{
			if (MainFileManager->getBufferByIndex(i)->isDirty())
			{
				isSeveralDirty = true;
				break;
			}
		}
	}

	bool isCurrentUntitled = curBuf->isUntitled();
	enableCommand(IDM_FILE_SAVE, isCurrentDirty, MENU | TOOLBAR);
	enableCommand(IDM_FILE_SAVEALL, isSeveralDirty, MENU | TOOLBAR);
	enableCommand(IDM_VIEW_GOTO_NEW_INSTANCE, !(isCurrentDirty || isCurrentUntitled), MENU);
	enableCommand(IDM_VIEW_LOAD_IN_NEW_INSTANCE, !(isCurrentDirty || isCurrentUntitled), MENU);
	
	bool isSysReadOnly = curBuf->getFileReadOnly();
	if (isSysReadOnly)
	{
		::CheckMenuItem(_mainMenuHandle, IDM_EDIT_SETREADONLY, MF_BYCOMMAND | MF_UNCHECKED);
		enableCommand(IDM_EDIT_SETREADONLY, false, MENU);
		enableCommand(IDM_EDIT_CLEARREADONLY, true, MENU);
	}
	else
	{
		enableCommand(IDM_EDIT_SETREADONLY, true, MENU);
		enableCommand(IDM_EDIT_CLEARREADONLY, false, MENU);
		bool isUserReadOnly = curBuf->getUserReadOnly();
		::CheckMenuItem(_mainMenuHandle, IDM_EDIT_SETREADONLY, MF_BYCOMMAND | (isUserReadOnly?MF_CHECKED:MF_UNCHECKED));
	}
	enableCommand(IDM_FILE_DELETE, isFileExisting, MENU);
	enableCommand(IDM_FILE_RENAME, isFileExisting, MENU);

	enableConvertMenuItems(curBuf->getFormat());
	checkUnicodeMenuItems(curBuf->getUnicodeMode());
	checkLangsMenu(-1);
}

void Notepad_plus::checkUndoState()
{
	enableCommand(IDM_EDIT_UNDO, _pEditView->execute(SCI_CANUNDO) != 0, MENU | TOOLBAR);
	enableCommand(IDM_EDIT_REDO, _pEditView->execute(SCI_CANREDO) != 0, MENU | TOOLBAR);
}

void Notepad_plus::checkMacroState()
{
	enableCommand(IDM_MACRO_STARTRECORDINGMACRO, !_recordingMacro, MENU | TOOLBAR);
	enableCommand(IDM_MACRO_STOPRECORDINGMACRO, _recordingMacro, MENU | TOOLBAR);
	enableCommand(IDM_MACRO_PLAYBACKRECORDEDMACRO, !_macro.empty() && !_recordingMacro, MENU | TOOLBAR);
	enableCommand(IDM_MACRO_SAVECURRENTMACRO, !_macro.empty() && !_recordingMacro, MENU | TOOLBAR);
	
	enableCommand(IDM_MACRO_RUNMULTIMACRODLG, (!_macro.empty() && !_recordingMacro) || !((NppParameters::getInstance())->getMacroList()).empty(), MENU | TOOLBAR);
}

void Notepad_plus::checkSyncState()
{
	bool canDoSync = viewVisible(MAIN_VIEW) && viewVisible(SUB_VIEW);
	if (!canDoSync)
	{
		_syncInfo._isSynScollV = false;
		_syncInfo._isSynScollH = false;
		checkMenuItem(IDM_VIEW_SYNSCROLLV, false);
		checkMenuItem(IDM_VIEW_SYNSCROLLH, false);
		_toolBar.setCheck(IDM_VIEW_SYNSCROLLV, false);
		_toolBar.setCheck(IDM_VIEW_SYNSCROLLH, false);
	}
	enableCommand(IDM_VIEW_SYNSCROLLV, canDoSync, MENU | TOOLBAR);
	enableCommand(IDM_VIEW_SYNSCROLLH, canDoSync, MENU | TOOLBAR);
}

void Notepad_plus::checkLangsMenu(int id) const 
{
	Buffer * curBuf = _pEditView->getCurrentBuffer();
	if (id == -1)
	{
		id = (NppParameters::getInstance())->langTypeToCommandID(curBuf->getLangType());
		if (id == IDM_LANG_USER)
		{
			if (curBuf->isUserDefineLangExt())
			{
				const TCHAR *userLangName = curBuf->getUserDefineLangName();
				TCHAR menuLangName[16];

				for (int i = IDM_LANG_USER + 1 ; i <= IDM_LANG_USER_LIMIT ; i++)
				{
					if (::GetMenuString(_mainMenuHandle, i, menuLangName, sizeof(menuLangName), MF_BYCOMMAND))
						if (!lstrcmp(userLangName, menuLangName))
						{
							::CheckMenuRadioItem(_mainMenuHandle, IDM_LANG_C, IDM_LANG_USER_LIMIT, i, MF_BYCOMMAND);
							return;
						}
				}
			}
		}
	}
	::CheckMenuRadioItem(_mainMenuHandle, IDM_LANG_C, IDM_LANG_USER_LIMIT, id, MF_BYCOMMAND);
}

generic_string Notepad_plus::getLangDesc(LangType langType, bool shortDesc)
{

	if ((langType >= L_EXTERNAL) && (langType < NppParameters::getInstance()->L_END))
	{
		ExternalLangContainer & elc = NppParameters::getInstance()->getELCFromIndex(langType - L_EXTERNAL);
		if (shortDesc)
			return generic_string(elc._name);
		else
			return generic_string(elc._desc);
	}

	generic_string str2Show = ScintillaEditView::langNames[langType].longName;

	if (langType == L_USER)
	{
		Buffer * currentBuf = _pEditView->getCurrentBuffer();
		if (currentBuf->isUserDefineLangExt())
		{
			str2Show += TEXT(" - ");
			str2Show += currentBuf->getUserDefineLangName();
		}
	}
	return str2Show;
}

BOOL Notepad_plus::notify(SCNotification *notification)
{
	//Important, keep track of which element generated the message
	bool isFromPrimary = (_mainEditView.getHSelf() == notification->nmhdr.hwndFrom || _mainDocTab.getHSelf() == notification->nmhdr.hwndFrom);
	bool isFromSecondary = !isFromPrimary && (_subEditView.getHSelf() == notification->nmhdr.hwndFrom || _subDocTab.getHSelf() == notification->nmhdr.hwndFrom);
	ScintillaEditView * notifyView = isFromPrimary?&_mainEditView:&_subEditView;
	DocTabView *notifyDocTab = isFromPrimary?&_mainDocTab:&_subDocTab;
	TBHDR * tabNotification = (TBHDR*) notification;
	switch (notification->nmhdr.code) 
	{
  
		case SCN_MODIFIED:
		{
			static bool prevWasEdit = false;
			if (notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
			{
				prevWasEdit = true;
				_linkTriggered = true;
				_isDocModifing = true;
				::InvalidateRect(notifyView->getHSelf(), NULL, TRUE);
			}
			if (notification->modificationType & SC_MOD_CHANGEFOLD)
			{
				if (prevWasEdit) {
					notifyView->foldChanged(notification->line,
							notification->foldLevelNow, notification->foldLevelPrev);
					prevWasEdit = false;
				}
			}
			else
			if (!(notification->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT)))
			{
				prevWasEdit = false;
			}
		}
		break;

		case SCN_SAVEPOINTREACHED:
		case SCN_SAVEPOINTLEFT:
		{
			Buffer * buf = 0;
			if (isFromPrimary) {
				buf = _mainEditView.getCurrentBuffer();
			} else if (isFromSecondary) {
				buf = _subEditView.getCurrentBuffer();
			} else {
				//Done by invisibleEditView?
				BufferID id = BUFFER_INVALID;
				if (notification->nmhdr.hwndFrom == _invisibleEditView.getHSelf()) {
					id = MainFileManager->getBufferFromDocument(_invisibleEditView.execute(SCI_GETDOCPOINTER));
				} else if (notification->nmhdr.hwndFrom == _fileEditView.getHSelf()) {
					id = MainFileManager->getBufferFromDocument(_fileEditView.execute(SCI_GETDOCPOINTER));
				} else {
					break;	//wrong scintilla
				}
				if (id != BUFFER_INVALID) {
					buf = MainFileManager->getBufferByID(id);
				} else {
					break;
				}
			}
			buf->setDirty(notification->nmhdr.code == SCN_SAVEPOINTLEFT);
			break; }

		case  SCN_MODIFYATTEMPTRO :
			// on fout rien
			break;

		case SCN_KEY:
			break;

	case TCN_TABDROPPEDOUTSIDE:
	case TCN_TABDROPPED:
	{
        TabBarPlus *sender = reinterpret_cast<TabBarPlus *>(notification->nmhdr.idFrom);

        if (notification->nmhdr.code == TCN_TABDROPPEDOUTSIDE)
        {
            POINT p = sender->getDraggingPoint();

			//It's the coordinate of screen, so we can call 
			//"WindowFromPoint" function without converting the point
            HWND hWin = ::WindowFromPoint(p);
			if (hWin == _pEditView->getHSelf()) // In the same view group
			{
				if (!_tabPopupDropMenu.isCreated())
				{
					TCHAR goToView[32] = TEXT("Go to another View");
					TCHAR cloneToView[32] = TEXT("Clone to another View");
					vector<MenuItemUnit> itemUnitArray;
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, goToView));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, cloneToView));
					_tabPopupDropMenu.create(_hSelf, itemUnitArray);
					changeLangTabDrapContextMenu();
				}
				_tabPopupDropMenu.display(p);
			}
			else if ((hWin == _pNonDocTab->getHSelf()) || 
				     (hWin == _pNonEditView->getHSelf())) // In the another view group
			{
				if (::GetKeyState(VK_LCONTROL) & 0x80000000)
					docGotoAnotherEditView(TransferClone);
				else
					docGotoAnotherEditView(TransferMove);
			}
			else
			{
				generic_string quotFileName = TEXT("\"");
				quotFileName += _pEditView->getCurrentBuffer()->getFullPathName();
				quotFileName += TEXT("\"");
				COPYDATASTRUCT fileNamesData;
				fileNamesData.dwData = COPYDATA_FILENAMES;
				fileNamesData.lpData = (void *)quotFileName.c_str();
				fileNamesData.cbData = long(quotFileName.length() + 1)*(sizeof(TCHAR));

				HWND hWinParent = ::GetParent(hWin);
				TCHAR className[MAX_PATH];
				::GetClassName(hWinParent,className, sizeof(className));
				if (lstrcmp(className, _className) == 0 && hWinParent != _hSelf)
				{
					int index = _pDocTab->getCurrentTabIndex();
					BufferID bufferToClose = notifyDocTab->getBufferByIndex(index);
					Buffer * buf = MainFileManager->getBufferByID(bufferToClose);
					int iView = isFromPrimary?MAIN_VIEW:SUB_VIEW;
					if (buf->isDirty()) 
					{
						::MessageBox(_hSelf, TEXT("Document is modified, save it then try again."), TEXT("Go to another Notepad++ instance"), MB_OK);
					}
					else
					{
						::SendMessage(hWinParent, NPPM_INTERNAL_SWITCHVIEWFROMHWND, 0, (LPARAM)hWin);
						::SendMessage(hWinParent, WM_COPYDATA, (WPARAM)_hInst, (LPARAM)&fileNamesData);
						fileClose(bufferToClose, iView);
					}
				}
			}
        }
		break;
	}

	case TCN_TABDELETE:
	{
		int index = tabNotification->tabOrigin;
		BufferID bufferToClose = notifyDocTab->getBufferByIndex(index);
		Buffer * buf = MainFileManager->getBufferByID(bufferToClose);
		int iView = isFromPrimary?MAIN_VIEW:SUB_VIEW;
		if (buf->isDirty()) {	//activate and use fileClose() (for save and abort)
			activateBuffer(bufferToClose, iView);
			fileClose(bufferToClose, iView);
			break;
		}
		int open = 1;
		if (isFromPrimary || isFromSecondary)
			open = notifyDocTab->nbItem();
		doClose(bufferToClose, iView);
		//if (open == 1 && canHideView(iView))
		//	hideView(iView);
		break;

	}

	case TCN_SELCHANGE:
	{
		int iView = -1;
        if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
		{
			iView = MAIN_VIEW;
		}
		else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
		{
			iView = SUB_VIEW;
		}
		else
		{
			break;
		}

		switchEditViewTo(iView);
		BufferID bufid = _pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex());
		if (bufid != BUFFER_INVALID)
			activateBuffer(bufid, iView);
		
		break;
	}

	case NM_CLICK :
    {        
		if (notification->nmhdr.hwndFrom == _statusBar.getHSelf())
        {
            LPNMMOUSE lpnm = (LPNMMOUSE)notification;
			if (lpnm->dwItemSpec == DWORD(STATUSBAR_TYPING_MODE))
			{
				bool isOverTypeMode = (_pEditView->execute(SCI_GETOVERTYPE) != 0);
				_pEditView->execute(SCI_SETOVERTYPE, !isOverTypeMode);
				_statusBar.setText((_pEditView->execute(SCI_GETOVERTYPE))?TEXT("OVR"):TEXT("INS"), STATUSBAR_TYPING_MODE);
			}
        }
		else if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
		{
            switchEditViewTo(MAIN_VIEW);
		}
        else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
        {
            switchEditViewTo(SUB_VIEW);
        }

		break;
	}

	case NM_DBLCLK :
    {        
		if (notification->nmhdr.hwndFrom == _statusBar.getHSelf())
        {
            LPNMMOUSE lpnm = (LPNMMOUSE)notification;
			if (lpnm->dwItemSpec == DWORD(STATUSBAR_CUR_POS))
			{
				bool isFirstTime = !_goToLineDlg.isCreated();
				_goToLineDlg.doDialog(_isRTL);
				if (isFirstTime)
					changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
			}
        }
		break;
	}

    case NM_RCLICK :
    {
		if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
		{
            switchEditViewTo(MAIN_VIEW);
		}
        else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
        {
            switchEditViewTo(SUB_VIEW);
        }
		else // From tool bar or Status Bar
			return TRUE;
			//break;
        
		POINT p;
		::GetCursorPos(&p);

		if (!_tabPopupMenu.isCreated())
		{
			vector<MenuItemUnit> itemUnitArray;
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSE, TEXT("Close me")));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_BUT_CURRENT, TEXT("Close all but me")));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVE, TEXT("Save me")));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVEAS, TEXT("Save me As...")));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_RENAME, TEXT("Rename me")));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_DELETE, TEXT("Delete me")));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_PRINT, TEXT("Print me")));
			itemUnitArray.push_back(MenuItemUnit(0, NULL));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_SETREADONLY, TEXT("Read only")));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CLEARREADONLY, TEXT("Clear read only flag")));
			itemUnitArray.push_back(MenuItemUnit(0, NULL));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FULLPATHTOCLIP,	TEXT("Full file path to Clipboard")));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FILENAMETOCLIP,   TEXT("File name to Clipboard")));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CURRENTDIRTOCLIP, TEXT("Current dir path to Clipboard")));
			itemUnitArray.push_back(MenuItemUnit(0, NULL));
			itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, TEXT("Go to another View")));
			itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, TEXT("Clone to another View")));
			itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_NEW_INSTANCE, TEXT("Go to new instance")));
			itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_LOAD_IN_NEW_INSTANCE, TEXT("Open in new instance")));

			_tabPopupMenu.create(_hSelf, itemUnitArray);
			changeLangTabContextMenu();
		}

		bool isEnable = ((::GetMenuState(_mainMenuHandle, IDM_FILE_SAVE, MF_BYCOMMAND)&MF_DISABLED) == 0);
		_tabPopupMenu.enableItem(IDM_FILE_SAVE, isEnable);
		
		Buffer * buf = _pEditView->getCurrentBuffer();
		bool isUserReadOnly = buf->getUserReadOnly();
		_tabPopupMenu.checkItem(IDM_EDIT_SETREADONLY, isUserReadOnly);

		bool isSysReadOnly = buf->getFileReadOnly();
		_tabPopupMenu.enableItem(IDM_EDIT_SETREADONLY, !isSysReadOnly);
		_tabPopupMenu.enableItem(IDM_EDIT_CLEARREADONLY, isSysReadOnly);

		bool isFileExisting = PathFileExists(buf->getFullPathName()) != FALSE;
		_tabPopupMenu.enableItem(IDM_FILE_DELETE, isFileExisting);
		_tabPopupMenu.enableItem(IDM_FILE_RENAME, isFileExisting);

		bool isDirty = buf->isDirty();
		bool isUntitled = buf->isUntitled();
		_tabPopupMenu.enableItem(IDM_VIEW_GOTO_NEW_INSTANCE, !(isDirty||isUntitled));
		_tabPopupMenu.enableItem(IDM_VIEW_LOAD_IN_NEW_INSTANCE, !(isDirty||isUntitled));

		_tabPopupMenu.display(p);
		return TRUE;
    }

    
	case SCN_MARGINCLICK:
    {
        if (notification->nmhdr.hwndFrom == _mainEditView.getHSelf())
            switchEditViewTo(MAIN_VIEW);
			
		else if (notification->nmhdr.hwndFrom == _subEditView.getHSelf())
            switchEditViewTo(SUB_VIEW);
        
        if (notification->margin == ScintillaEditView::_SC_MARGE_FOLDER)
        {
            _pEditView->marginClick(notification->position, notification->modifiers);
        }
        else if ((notification->margin == ScintillaEditView::_SC_MARGE_SYBOLE) && !notification->modifiers)
        {
            
            int lineClick = int(_pEditView->execute(SCI_LINEFROMPOSITION, notification->position));
			if (!_pEditView->markerMarginClick(lineClick))
				bookmarkToggle(lineClick);
        
        }
		break;
	}
	
	case SCN_CHARADDED:
	{
		charAdded(static_cast<TCHAR>(notification->ch));

		AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;

		autoC->update(notification->ch);
		
		break;
	}

	case SCN_DOUBLECLICK :
	{
		if (_isHotspotDblClicked)
		{
			int pos = notifyView->execute(SCI_GETCURRENTPOS);
			notifyView->execute(SCI_SETCURRENTPOS, pos);
			notifyView->execute(SCI_SETANCHOR, pos);
			_isHotspotDblClicked = false;
		}
	}
	break;

    case SCN_UPDATEUI:
	{
		if (notification->nmhdr.hwndFrom != _pEditView->getHSelf())
			break;

        braceMatch();

		const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();
		if (nppGUI._enableTagsMatchHilite/* && _pEditView->getHSelf() == notification->nmhdr.hwndFrom*/)
		{
			XmlMatchedTagsHighlighter xmlTagMatchHiliter(_pEditView);
			xmlTagMatchHiliter.tagMatch(nppGUI._enableTagAttrsHilite);
		}
		
		_smartHighlighter.highlightView(notifyView);

		updateStatusBar();
		AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
		autoC->update(0);
        break;
	}

	case SCN_SCROLLED:
	{
		_smartHighlighter.highlightView(notifyView);
		break;
	}

    case TTN_GETDISPINFO:
    { 
        LPTOOLTIPTEXT lpttt; 

        lpttt = (LPTOOLTIPTEXT)notification; 
        lpttt->hinst = _hInst; 

		POINT p;
		::GetCursorPos(&p);
		::ScreenToClient(_hSelf, &p);
		HWND hWin = ::RealChildWindowFromPoint(_hSelf, p);

		static generic_string tip;
		int id = int(lpttt->hdr.idFrom);

		if (hWin == _rebarTop.getHSelf())
		{
			getNameStrFromCmd(id, tip);
		}
		else if (hWin == _mainDocTab.getHSelf())
		{
			BufferID idd = _mainDocTab.getBufferByIndex(id);
			Buffer * buf = MainFileManager->getBufferByID(idd);
			tip = buf->getFullPathName();
		}
		else if (hWin == _subDocTab.getHSelf())
		{
			BufferID idd = _subDocTab.getBufferByIndex(id);
			Buffer * buf = MainFileManager->getBufferByID(idd);
			tip = buf->getFullPathName();
		}
		else
			break;

		lpttt->lpszText = (TCHAR *)tip.c_str();
    } 
    break;

    case SCN_ZOOM:
        notifyView->setLineNumberWidth(notifyView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER));
		break;

    case SCN_MACRORECORD:
        _macro.push_back(recordedMacroStep(notification->message, notification->wParam, notification->lParam));
		break;

	case SCN_PAINTED:
	{
		if (_syncInfo.doSync()) 
			doSynScorll(HWND(notification->nmhdr.hwndFrom));

		if (_linkTriggered)
		{
			int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
			if ((urlAction == 1) || (urlAction == 2))
				addHotSpot(_isDocModifing);
			_linkTriggered = false;
			_isDocModifing = false;
		}
		break;
	}

	case SCN_HOTSPOTDOUBLECLICK :
	{
		notifyView->execute(SCI_SETWORDCHARS, 0, (LPARAM)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-+.:?&@=/%#");
		
		int pos = notifyView->execute(SCI_GETCURRENTPOS);
		int startPos = static_cast<int>(notifyView->execute(SCI_WORDSTARTPOSITION, pos, false));
		int endPos = static_cast<int>(notifyView->execute(SCI_WORDENDPOSITION, pos, false));

		notifyView->execute(SCI_SETTARGETSTART, startPos);
		notifyView->execute(SCI_SETTARGETEND, endPos);
	
		int posFound = notifyView->execute(SCI_SEARCHINTARGET, strlen(urlHttpRegExpr), (LPARAM)urlHttpRegExpr);
		if (posFound != -1)
		{
			startPos = int(notifyView->execute(SCI_GETTARGETSTART));
			endPos = int(notifyView->execute(SCI_GETTARGETEND));
		}

		TCHAR currentWord[MAX_PATH*2];
		notifyView->getGenericText(currentWord, startPos, endPos);

		::ShellExecute(_hSelf, TEXT("open"), currentWord, NULL, NULL, SW_SHOW);
		_isHotspotDblClicked = true;
		notifyView->execute(SCI_SETCHARSDEFAULT);
		break;
	}

	case SCN_NEEDSHOWN :
	{
		int begin = notifyView->execute(SCI_LINEFROMPOSITION, notification->position);
		int end = notifyView->execute(SCI_LINEFROMPOSITION, notification->position + notification->length);
		int firstLine = begin < end ? begin : end;
		int lastLine = begin > end ? begin : end;
		for (int line = firstLine; line <= lastLine; line++) {
			notifyView->execute(SCI_ENSUREVISIBLE, line, 0);
		}
		break;
	}

	case SCN_CALLTIPCLICK:
	{
		AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
		autoC->callTipClick(notification->position);
		break;
	}

	case RBN_HEIGHTCHANGE:
	{
		SendMessage(_hSelf, WM_SIZE, 0, 0);
		break;
	}
	case RBN_CHEVRONPUSHED:
	{
		NMREBARCHEVRON * lpnm = (NMREBARCHEVRON*) notification;
		ReBar * notifRebar = &_rebarTop;
		if (_rebarBottom.getHSelf() == lpnm->hdr.hwndFrom)
			notifRebar = &_rebarBottom;
		//If N++ ID, use proper object
		switch(lpnm->wID) {
			case REBAR_BAR_TOOLBAR: {
				POINT pt;
				pt.x = lpnm->rc.left;
				pt.y = lpnm->rc.bottom;
				ClientToScreen(notifRebar->getHSelf(), &pt);
				_toolBar.doPopop(pt);
				return TRUE;
				break; }
		}
		//Else forward notification to window of rebarband
		REBARBANDINFO rbBand;
		rbBand.cbSize = sizeof(rbBand);
		rbBand.fMask = RBBIM_CHILD;
		::SendMessage(notifRebar->getHSelf(), RB_GETBANDINFO, lpnm->uBand, (LPARAM)&rbBand);
		::SendMessage(rbBand.hwndChild, WM_NOTIFY, 0, (LPARAM)lpnm);
		break;
	}

	default :
		break;

  }
  return FALSE;
}

void Notepad_plus::findMatchingBracePos(int & braceAtCaret, int & braceOpposite)
{
	int caretPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	braceAtCaret = -1;
	braceOpposite = -1;
	TCHAR charBefore = '\0';
	//TCHAR styleBefore = '\0';
	int lengthDoc = int(_pEditView->execute(SCI_GETLENGTH));

	if ((lengthDoc > 0) && (caretPos > 0)) 
    {
		charBefore = TCHAR(_pEditView->execute(SCI_GETCHARAT, caretPos - 1, 0));
	}
	// Priority goes to character before caret
	if (charBefore && generic_strchr(TEXT("[](){}"), charBefore))
    {
		braceAtCaret = caretPos - 1;
	}

	if (lengthDoc > 0  && (braceAtCaret < 0)) 
    {
		// No brace found so check other side
		TCHAR charAfter = TCHAR(_pEditView->execute(SCI_GETCHARAT, caretPos, 0));
		if (charAfter && generic_strchr(TEXT("[](){}"), charAfter))
        {
			braceAtCaret = caretPos;
		}
	}
	if (braceAtCaret >= 0) 
		braceOpposite = int(_pEditView->execute(SCI_BRACEMATCH, braceAtCaret, 0));
}


void Notepad_plus::braceMatch() 
{
	int braceAtCaret = -1;
	int braceOpposite = -1;
	findMatchingBracePos(braceAtCaret, braceOpposite);

	if ((braceAtCaret != -1) && (braceOpposite == -1))
    {
		_pEditView->execute(SCI_BRACEBADLIGHT, braceAtCaret);
		_pEditView->execute(SCI_SETHIGHLIGHTGUIDE);
	} 
    else 
    {
		_pEditView->execute(SCI_BRACEHIGHLIGHT, braceAtCaret, braceOpposite);

		if (_pEditView->isShownIndentGuide())
        {
            int columnAtCaret = int(_pEditView->execute(SCI_GETCOLUMN, braceAtCaret));
		    int columnOpposite = int(_pEditView->execute(SCI_GETCOLUMN, braceOpposite));
			_pEditView->execute(SCI_SETHIGHLIGHTGUIDE, (columnAtCaret < columnOpposite)?columnAtCaret:columnOpposite);
        }
    }

    enableCommand(IDM_SEARCH_GOTOMATCHINGBRACE, (braceAtCaret != -1) && (braceOpposite != -1), MENU | TOOLBAR);
}

void Notepad_plus::charAdded(TCHAR chAdded)
{
	bool indentMaintain = NppParameters::getInstance()->getNppGUI()._maitainIndent;
	if (indentMaintain)
		MaintainIndentation(chAdded);
}

void Notepad_plus::addHotSpot(bool docIsModifing)
{
	//bool docIsModifing = true;
	int posBegin2style = 0;
	if (docIsModifing)
		posBegin2style = _pEditView->execute(SCI_GETCURRENTPOS);

	int endStyle = _pEditView->execute(SCI_GETENDSTYLED);
	if (docIsModifing)
	{

 		posBegin2style = _pEditView->execute(SCI_GETCURRENTPOS);
		if (posBegin2style > 0) posBegin2style--;
		UCHAR ch = (UCHAR)_pEditView->execute(SCI_GETCHARAT, posBegin2style);

		// determinating the type of EOF to make sure how many steps should we be back
		if ((ch == 0x0A) || (ch == 0x0D))
		{
			int eolMode = _pEditView->execute(SCI_GETEOLMODE);
			
			if ((eolMode == SC_EOL_CRLF) && (posBegin2style > 1))
				posBegin2style -= 2;
			else if (posBegin2style > 0)
				posBegin2style -= 1;
		}

		ch = (UCHAR)_pEditView->execute(SCI_GETCHARAT, posBegin2style);
		while ((posBegin2style > 0) && ((ch != 0x0A) && (ch != 0x0D)))
		{
			ch = (UCHAR)_pEditView->execute(SCI_GETCHARAT, posBegin2style--);
		}
	}
	int style_hotspot = 30;

	int startPos = 0;
	int endPos = _pEditView->execute(SCI_GETTEXTLENGTH);

	_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

	_pEditView->execute(SCI_SETTARGETSTART, startPos);
	_pEditView->execute(SCI_SETTARGETEND, endPos);

	vector<pair<int, int> > hotspotStylers;
	
	int posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(urlHttpRegExpr), (LPARAM)urlHttpRegExpr);

	while (posFound != -1)
	{
		int start = int(_pEditView->execute(SCI_GETTARGETSTART));
		int end = int(_pEditView->execute(SCI_GETTARGETEND));
		int foundTextLen = end - start;
		int idStyle = _pEditView->execute(SCI_GETSTYLEAT, posFound);

		if (end < posBegin2style - 1)
		{
			if (style_hotspot > 1)
				style_hotspot--;
		}
		else
		{
			int fs = -1;
			for (size_t i = 0 ; i < hotspotStylers.size() ; i++)
			{
				if (hotspotStylers[i].second == idStyle)
				{
					fs = hotspotStylers[i].first;
					break;
				}
			}

			if (fs != -1)
			{
				_pEditView->execute(SCI_STARTSTYLING, start, 0xFF);
				_pEditView->execute(SCI_SETSTYLING, foundTextLen, fs);

			}
			else
			{
				pair<int, int> p(style_hotspot, idStyle);
				hotspotStylers.push_back(p);
				int activeFG = 0xFF0000;

				//TCHAR fontName[256];
				Style hotspotStyle;
				
				hotspotStyle._styleID = style_hotspot;
				//_pEditView->execute(SCI_STYLEGETFONT, idStyle, (LPARAM)fontName);
				hotspotStyle._fgColor = _pEditView->execute(SCI_STYLEGETFORE, idStyle);
				hotspotStyle._bgColor = _pEditView->execute(SCI_STYLEGETBACK, idStyle);
				hotspotStyle._fontSize = _pEditView->execute(SCI_STYLEGETSIZE, idStyle);

				int isBold = _pEditView->execute(SCI_STYLEGETBOLD, idStyle);
				int isItalic = _pEditView->execute(SCI_STYLEGETITALIC, idStyle);
				int isUnderline = _pEditView->execute(SCI_STYLEGETUNDERLINE, idStyle);
				hotspotStyle._fontStyle = (isBold?FONTSTYLE_BOLD:0) | (isItalic?FONTSTYLE_ITALIC:0) | (isUnderline?FONTSTYLE_UNDERLINE:0);

				int fontStyle = (isBold?FONTSTYLE_BOLD:0) | (isItalic?FONTSTYLE_ITALIC:0) | (isUnderline?FONTSTYLE_UNDERLINE:0);
				int urlAction = (NppParameters::getInstance())->getNppGUI()._styleURL;
				if (urlAction == 2)
					hotspotStyle._fontStyle |= FONTSTYLE_UNDERLINE;

				_pEditView->setStyle(hotspotStyle);

				_pEditView->execute(SCI_STYLESETHOTSPOT, style_hotspot, TRUE);
				_pEditView->execute(SCI_SETHOTSPOTACTIVEFORE, TRUE, activeFG);
				_pEditView->execute(SCI_SETHOTSPOTSINGLELINE, style_hotspot, 0);
				_pEditView->execute(SCI_STARTSTYLING, start, 0x1F);
				_pEditView->execute(SCI_SETSTYLING, foundTextLen, style_hotspot);
				if (style_hotspot > 1)
					style_hotspot--;	
			}
		}

		_pEditView->execute(SCI_SETTARGETSTART, posFound + foundTextLen);
		_pEditView->execute(SCI_SETTARGETEND, endPos);
		
		
		posFound = _pEditView->execute(SCI_SEARCHINTARGET, strlen(urlHttpRegExpr), (LPARAM)urlHttpRegExpr);
	}


	_pEditView->execute(SCI_STARTSTYLING, endStyle, 0xFF);
	_pEditView->execute(SCI_SETSTYLING, 0, 0);
}



void Notepad_plus::MaintainIndentation(TCHAR ch)
{
	int eolMode = int(_pEditView->execute(SCI_GETEOLMODE));
	int curLine = int(_pEditView->getCurrentLineNumber());
	int lastLine = curLine - 1;
	int indentAmount = 0;

	if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
	        (eolMode == SC_EOL_CR && ch == '\r')) 
	{
		while (lastLine >= 0 && _pEditView->getLineLength(lastLine) == 0)
			lastLine--;

		if (lastLine >= 0) {
			indentAmount = _pEditView->getLineIndent(lastLine);
		}
		if (indentAmount > 0) {
			_pEditView->setLineIndent(curLine, indentAmount);
		}
	}
}
void Notepad_plus::specialCmd(int id, int param)
{	
	if ((param != 1) && (param != 2)) return;

	NppParameters *pNppParam = NppParameters::getInstance();
	ScintillaEditView *pEditView = (param == 1)?&_mainEditView:&_subEditView;

	switch (id)
	{
        case IDM_VIEW_LINENUMBER:
        case IDM_VIEW_SYMBOLMARGIN:
        case IDM_VIEW_FOLDERMAGIN:
        {
            int margin;
            if (id == IDM_VIEW_LINENUMBER)
                margin = ScintillaEditView::_SC_MARGE_LINENUMBER;
            else if (id == IDM_VIEW_SYMBOLMARGIN)
                margin = ScintillaEditView::_SC_MARGE_SYBOLE;
            else
                margin = ScintillaEditView::_SC_MARGE_FOLDER;

            if (pEditView->hasMarginShowed(margin))
                pEditView->showMargin(margin, false);
            else
                pEditView->showMargin(margin);

			break;
        }

        case IDM_VIEW_FOLDERMAGIN_SIMPLE:
        case IDM_VIEW_FOLDERMAGIN_ARROW:
        case IDM_VIEW_FOLDERMAGIN_CIRCLE:
        case IDM_VIEW_FOLDERMAGIN_BOX:
        {
            int checkedID = getFolderMarginStyle();
            if (checkedID == id) return;
            folderStyle fStyle = (id == IDM_VIEW_FOLDERMAGIN_SIMPLE)?FOLDER_STYLE_SIMPLE:\
                ((id == IDM_VIEW_FOLDERMAGIN_ARROW)?FOLDER_STYLE_ARROW:\
                ((id == IDM_VIEW_FOLDERMAGIN_CIRCLE)?FOLDER_STYLE_CIRCLE:FOLDER_STYLE_BOX));
            pEditView->setMakerStyle(fStyle);
            break;
        }
		
		case IDM_VIEW_CURLINE_HILITING:
		{
            COLORREF colour = pNppParam->getCurLineHilitingColour();
			pEditView->setCurrentLineHiLiting(!_pEditView->isCurrentLineHiLiting(), colour);
			break;
		}
		
		case IDM_VIEW_EDGEBACKGROUND:
		case IDM_VIEW_EDGELINE:
		case IDM_VIEW_EDGENONE:
		{
			int mode;
			switch (id)
			{
				case IDM_VIEW_EDGELINE:
				{
					mode = EDGE_LINE;
					break;
				}
				case IDM_VIEW_EDGEBACKGROUND:
				{
					mode = EDGE_BACKGROUND;
					break;
				}
				default :
					mode = EDGE_NONE;
			}
			pEditView->execute(SCI_SETEDGEMODE, mode);
			break;
		}

		case IDM_SETTING_EDGE_SIZE :
		{
			ValueDlg nbColumnEdgeDlg;
			ScintillaViewParams & svp = (ScintillaViewParams &)pNppParam->getSVP(param == 1?SCIV_PRIMARY:SCIV_SECOND);
			nbColumnEdgeDlg.init(_hInst, _preference.getHSelf(), svp._edgeNbColumn, TEXT("Nb of column:"));
			nbColumnEdgeDlg.setNBNumber(3);

			POINT p;
			::GetCursorPos(&p);
			::ScreenToClient(_hParent, &p);
			int size = nbColumnEdgeDlg.doDialog(p, _isRTL);

			if (size != -1)
			{
				svp._edgeNbColumn = size;
				pEditView->execute(SCI_SETEDGECOLUMN, size);
			}
			break;
		}
	}
}

void Notepad_plus::command(int id) 
{
	NppParameters *pNppParam = NppParameters::getInstance();
	switch (id)
	{
		case IDM_FILE_NEW:
		{
			fileNew();
		}
		break;
		
		case IDM_FILE_OPEN:
			fileOpen();
			break;

		case IDM_FILE_RELOAD:
			fileReload();
			break;

		case IDM_FILE_CLOSE:
			fileClose();
			break;

		case IDM_FILE_DELETE:
			fileDelete();
			break;

		case IDM_FILE_RENAME:
			fileRename();
			break;

		case IDM_FILE_CLOSEALL:
			fileCloseAll();
			break;

		case IDM_FILE_CLOSEALL_BUT_CURRENT :
			fileCloseAllButCurrent();
			break;

		case IDM_FILE_SAVE :
			fileSave();
			break;

		case IDM_FILE_SAVEALL :
			fileSaveAll();
			break;

		case IDM_FILE_SAVEAS :
			fileSaveAs();
			break;

		case IDM_FILE_SAVECOPYAS :
			fileSaveAs(BUFFER_INVALID, true);
			break;

		case IDM_FILE_LOADSESSION:
			fileLoadSession();
			break;

		case IDM_FILE_SAVESESSION:
			fileSaveSession();
			break;

		case IDM_FILE_PRINTNOW :
			filePrint(false);
			break;

		case IDM_FILE_PRINT :
			filePrint(true);
			break;

		case IDM_FILE_EXIT:
			::SendMessage(_hSelf, WM_CLOSE, 0, 0);
			break;

		case IDM_EDIT_UNDO:
			_pEditView->execute(WM_UNDO);
			checkClipboard();
			checkUndoState();
			break;

		case IDM_EDIT_REDO:
			_pEditView->execute(SCI_REDO);
			checkClipboard();
			checkUndoState();
			break;

		case IDM_EDIT_CUT:
			_pEditView->execute(WM_CUT);
			checkClipboard();
			break;

		case IDM_EDIT_COPY:
			_pEditView->execute(WM_COPY);
			checkClipboard();
			break;

		case IDM_EDIT_PASTE:
		{
			int eolMode = int(_pEditView->execute(SCI_GETEOLMODE));
			_pEditView->execute(SCI_PASTE);
			_pEditView->execute(SCI_CONVERTEOLS, eolMode);
		}
		break;

		case IDM_EDIT_DELETE:
			_pEditView->execute(WM_CLEAR);
			break;

		case IDM_MACRO_STARTRECORDINGMACRO:
		case IDM_MACRO_STOPRECORDINGMACRO:
		case IDC_EDIT_TOGGLEMACRORECORDING:
		{
			//static HCURSOR originalCur;

			if (_recordingMacro)
			{
				// STOP !!!
				_mainEditView.execute(SCI_STOPRECORD);
				//_mainEditView.execute(SCI_ENDUNDOACTION);
				_subEditView.execute(SCI_STOPRECORD);
				//_subEditView.execute(SCI_ENDUNDOACTION);
				
				//::SetCursor(originalCur);
				_mainEditView.execute(SCI_SETCURSOR, (LPARAM)SC_CURSORNORMAL);
				_subEditView.execute(SCI_SETCURSOR, (LPARAM)SC_CURSORNORMAL);
				
				_recordingMacro = false;
				_runMacroDlg.initMacroList();
			}
			else
			{
				//originalCur = ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_MACRO_RECORDING));
				//::SetCursor(originalCur);
				_mainEditView.execute(SCI_SETCURSOR, 9);
				_subEditView.execute(SCI_SETCURSOR, 9);
				_macro.clear();

				// START !!!
				_mainEditView.execute(SCI_STARTRECORD);
				//_mainEditView.execute(SCI_BEGINUNDOACTION);

				_subEditView.execute(SCI_STARTRECORD);
				//_subEditView.execute(SCI_BEGINUNDOACTION);
				_recordingMacro = true;
			}
			checkMacroState();
			break;
		}

		case IDM_MACRO_PLAYBACKRECORDEDMACRO:
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				_pEditView->execute(SCI_BEGINUNDOACTION);

				for (Macro::iterator step = _macro.begin(); step != _macro.end(); step++)
					step->PlayBack(this, _pEditView);

				_pEditView->execute(SCI_ENDUNDOACTION);
			}
			break;

		case IDM_MACRO_RUNMULTIMACRODLG :
		{
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				bool isFirstTime = !_runMacroDlg.isCreated();
				_runMacroDlg.doDialog(_isRTL);
				
				if (isFirstTime)
				{
					changeDlgLang(_runMacroDlg.getHSelf(), "MultiMacro");
				}
				break;
				
			}
		}
		break;

		case IDM_MACRO_SAVECURRENTMACRO :
		{
			if (addCurrentMacro())
				_runMacroDlg.initMacroList();
			break;
		}
		case IDM_EDIT_FULLPATHTOCLIP :
		case IDM_EDIT_CURRENTDIRTOCLIP :
		case IDM_EDIT_FILENAMETOCLIP :
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			if (id == IDM_EDIT_FULLPATHTOCLIP) {
				str2Cliboard(buf->getFullPathName());
			} else if (id == IDM_EDIT_CURRENTDIRTOCLIP) {
				TCHAR dir[MAX_PATH];
				lstrcpy(dir, buf->getFullPathName());
				PathRemoveFileSpec((TCHAR *)dir);
				str2Cliboard(dir);
			} else if (id == IDM_EDIT_FILENAMETOCLIP) {
				str2Cliboard(buf->getFileName());
			}
		}
		break;

		case IDM_SEARCH_FIND :
		case IDM_SEARCH_REPLACE :
		{
			const int strSize = 64;
			TCHAR str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();

			_findReplaceDlg.doDialog((id == IDM_SEARCH_FIND)?FIND_DLG:REPLACE_DLG, _isRTL);

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str, _pEditView->getCurrentBuffer()->getUnicodeMode() != uni8Bit);

			if (isFirstTime)
				changeFindReplaceDlgLang();
			break;
		}

		case IDM_SEARCH_FINDINFILES :
		{
			::SendMessage(_hSelf, NPPM_LAUNCHFINDINFILESDLG, 0, 0);
			break;
		}
		case IDM_SEARCH_FINDINCREMENT :
		{
			const int strSize = 64;
			TCHAR str[strSize];

			_pEditView->getGenericSelectedText(str, strSize);
			_incrementFindDlg.setSearchText(str, _pEditView->getCurrentBuffer()->getUnicodeMode() != uni8Bit);

			_incrementFindDlg.display();
		}
		break;

		case IDM_SEARCH_FINDNEXT :
		case IDM_SEARCH_FINDPREV :
		{
			if (!_findReplaceDlg.isCreated())
				return;

			FindOption op = _findReplaceDlg.getCurrentOptions();
			op._whichDirection = (id == IDM_SEARCH_FINDNEXT?DIR_DOWN:DIR_UP);
			generic_string s = _findReplaceDlg.getText2search();

			_findReplaceDlg.processFindNext(s.c_str(), &op);
			break;
		}
		break;

		case IDM_SEARCH_VOLATILE_FINDNEXT :
		case IDM_SEARCH_VOLATILE_FINDPREV :
		{
			TCHAR text2Find[MAX_PATH];
			_pEditView->getGenericSelectedText(text2Find, MAX_PATH);

			FindOption op;
			op._isWholeWord = false;
			op._whichDirection = (id == IDM_SEARCH_VOLATILE_FINDNEXT?DIR_DOWN:DIR_UP);
			_findReplaceDlg.processFindNext(text2Find, &op);
			break;
		}
		case IDM_SEARCH_MARKALL :
		{
			const int strSize = 64;
			TCHAR text2Find[strSize];
			_pEditView->getGenericSelectedText(text2Find, strSize);

			FindOption op;
			op._isWholeWord = false;
			//op._whichDirection = (id == IDM_SEARCH_VOLATILE_FINDNEXT?DIR_DOWN:DIR_UP);
			_findReplaceDlg.markAll(text2Find);

			break;
		}

		case IDM_SEARCH_UNMARKALL :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE);
			break;
		}

        case IDM_SEARCH_GOTOLINE :
		{
			bool isFirstTime = !_goToLineDlg.isCreated();
			_goToLineDlg.doDialog(_isRTL);
			if (isFirstTime)
				changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
			break;
		}

        case IDM_EDIT_COLUMNMODE :
		{
			bool isFirstTime = !_colEditorDlg.isCreated();
			_colEditorDlg.doDialog(_isRTL);
			if (isFirstTime)
				changeDlgLang(_colEditorDlg.getHSelf(), "ColumnEditor");
			break;
		}

		case IDM_SEARCH_GOTOMATCHINGBRACE :
		{
			int braceAtCaret = -1;
			int braceOpposite = -1;
			findMatchingBracePos(braceAtCaret, braceOpposite);

			if (braceOpposite != -1)
				_pEditView->execute(SCI_GOTOPOS, braceOpposite);
			break;
		}

        case IDM_SEARCH_TOGGLE_BOOKMARK :
	        bookmarkToggle(-1);
            break;

	    case IDM_SEARCH_NEXT_BOOKMARK:
		    bookmarkNext(true);
		    break;

	    case IDM_SEARCH_PREV_BOOKMARK:
		    bookmarkNext(false);
		    break;

	    case IDM_SEARCH_CLEAR_BOOKMARKS:
			bookmarkClearAll();
		    break;
			
        case IDM_VIEW_USER_DLG :
        {
		    bool isUDDlgVisible = false;
                
		    UserDefineDialog *udd = _pEditView->getUserDefineDlg();

		    if (!udd->isCreated())
		    {
			    _pEditView->doUserDefineDlg(true, _isRTL);
				changeUserDefineLang();
				if (_isUDDocked)
					::SendMessage(udd->getHSelf(), WM_COMMAND, IDC_DOCK_BUTTON, 0);

		    }
			else
			{
				isUDDlgVisible = udd->isVisible();
				bool isUDDlgDocked = udd->isDocked();

				if ((isUDDlgDocked)&&(isUDDlgVisible))
				{
					::ShowWindow(_pMainSplitter->getHSelf(), SW_HIDE);

					if (bothActive())
						_pMainWindow = &_subSplitter;
					else
						_pMainWindow = _pDocTab;

					::SendMessage(_hSelf, WM_SIZE, 0, 0);
					
					udd->display(false);
					_mainWindowStatus &= ~WindowUserActive;
				}
				else if ((isUDDlgDocked)&&(!isUDDlgVisible))
				{
                    if (!_pMainSplitter)
                    {
                        _pMainSplitter = new SplitterContainer;
                        _pMainSplitter->init(_hInst, _hSelf);

                        Window *pWindow;
                        if (bothActive())
                            pWindow = &_subSplitter;
                        else
                            pWindow = _pDocTab;

                        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, RIGHT_FIX, 45); 
                    }

					_pMainWindow = _pMainSplitter;

					_pMainSplitter->setWin0((bothActive())?(Window *)&_subSplitter:(Window *)_pDocTab);

					::SendMessage(_hSelf, WM_SIZE, 0, 0);
					_pMainWindow->display();

					_mainWindowStatus |= WindowUserActive;
				}
				else if ((!isUDDlgDocked)&&(isUDDlgVisible))
				{
					udd->display(false);
				}
				else //((!isUDDlgDocked)&&(!isUDDlgVisible))
					udd->display();
			}
			checkMenuItem(IDM_VIEW_USER_DLG, !isUDDlgVisible);
			_toolBar.setCheck(IDM_VIEW_USER_DLG, !isUDDlgVisible);

            break;
        }

		case IDM_EDIT_SELECTALL:
			_pEditView->execute(SCI_SELECTALL);
			checkClipboard();
			break;

		case IDM_EDIT_INS_TAB:
			_pEditView->execute(SCI_TAB);
			break;

		case IDM_EDIT_RMV_TAB:
			_pEditView->execute(SCI_BACKTAB);
			break;

		case IDM_EDIT_DUP_LINE:
			_pEditView->execute(SCI_LINEDUPLICATE);
			break;

		case IDM_EDIT_SPLIT_LINES:
			_pEditView->execute(SCI_TARGETFROMSELECTION);
			_pEditView->execute(SCI_LINESSPLIT);
			break;

		case IDM_EDIT_JOIN_LINES:
			_pEditView->execute(SCI_TARGETFROMSELECTION);
			_pEditView->execute(SCI_LINESJOIN);
			break;

		case IDM_EDIT_LINE_UP:
			_pEditView->currentLineUp();
			break;

		case IDM_EDIT_LINE_DOWN:
			_pEditView->currentLineDown();
			break;

		case IDM_EDIT_UPPERCASE:
            _pEditView->convertSelectedTextToUpperCase();
			break;

		case IDM_EDIT_LOWERCASE:
            _pEditView->convertSelectedTextToLowerCase();
			break;

		case IDM_EDIT_BLOCK_COMMENT:
			doBlockComment(cm_toggle);
 			break;
 
		case IDM_EDIT_BLOCK_COMMENT_SET:
			doBlockComment(cm_comment);
			break;

		case IDM_EDIT_BLOCK_UNCOMMENT:
			doBlockComment(cm_uncomment);
			break;

		case IDM_EDIT_STREAM_COMMENT:
			doStreamComment();
			break;

		case IDM_EDIT_TRIMTRAILING:
			doTrimTrailing();
			break;

		case IDM_EDIT_SETREADONLY:
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			buf->setUserReadOnly(!buf->getUserReadOnly());
		}
		break;

		case IDM_EDIT_CLEARREADONLY:
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			
			DWORD dwFileAttribs = ::GetFileAttributes(buf->getFullPathName());
			dwFileAttribs ^= FILE_ATTRIBUTE_READONLY; 

			::SetFileAttributes(buf->getFullPathName(), dwFileAttribs); 

			buf->setFileReadOnly(false);
		}
		break;

		case IDM_SEARCH_CUTMARKEDLINES :
			cutMarkedLines();
			break;

		case IDM_SEARCH_COPYMARKEDLINES :
			copyMarkedLines();
			break;

		case IDM_SEARCH_PASTEMARKEDLINES :
			pasteToMarkedLines();
			break;

		case IDM_SEARCH_DELETEMARKEDLINES :
			deleteMarkedLines();
			break;

		case IDM_VIEW_FULLSCREENTOGGLE :
			fullScreenToggle();
			break;

	    case IDM_VIEW_ALWAYSONTOP:
		{
			int check = (::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED)?MF_UNCHECKED:MF_CHECKED;
			::CheckMenuItem(_mainMenuHandle, id, MF_BYCOMMAND | check);
			SetWindowPos(_hSelf, check == MF_CHECKED?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		}
		break;


		case IDM_VIEW_FOLD_CURRENT :
		case IDM_VIEW_UNFOLD_CURRENT :
			_pEditView->foldCurrentPos((id==IDM_VIEW_FOLD_CURRENT)?fold_collapse:fold_uncollapse);
			break;

		case IDM_VIEW_TOGGLE_FOLDALL:
		case IDM_VIEW_TOGGLE_UNFOLDALL:
		{
			_pEditView->foldAll((id==IDM_VIEW_TOGGLE_FOLDALL)?fold_collapse:fold_uncollapse);
		}
		break;

		case IDM_VIEW_FOLD_1:
		case IDM_VIEW_FOLD_2:
		case IDM_VIEW_FOLD_3:
		case IDM_VIEW_FOLD_4:
		case IDM_VIEW_FOLD_5:
		case IDM_VIEW_FOLD_6:
		case IDM_VIEW_FOLD_7:
		case IDM_VIEW_FOLD_8:
			_pEditView->collapse(id - IDM_VIEW_FOLD - 1, fold_collapse);
			break;

		case IDM_VIEW_UNFOLD_1:
		case IDM_VIEW_UNFOLD_2:
		case IDM_VIEW_UNFOLD_3:
		case IDM_VIEW_UNFOLD_4:
		case IDM_VIEW_UNFOLD_5:
		case IDM_VIEW_UNFOLD_6:
		case IDM_VIEW_UNFOLD_7:
		case IDM_VIEW_UNFOLD_8:
			_pEditView->collapse(id - IDM_VIEW_UNFOLD - 1, fold_uncollapse);
			break;


		case IDM_VIEW_TOOLBAR_REDUCE:
		{
            toolBarStatusType state = _toolBar.getState();

            if (state != TB_SMALL)
            {
			    _toolBar.reduce();
			    changeToolBarIcons();
            }
		}
		break;

		case IDM_VIEW_TOOLBAR_ENLARGE:
		{
            toolBarStatusType state = _toolBar.getState();

            if (state != TB_LARGE)
            {
			    _toolBar.enlarge();
			    changeToolBarIcons();
            }
		}
		break;

		case IDM_VIEW_TOOLBAR_STANDARD:
		{
			toolBarStatusType state = _toolBar.getState();

            if (state != TB_STANDARD)
            {
				_toolBar.setToUglyIcons();
			}
		}
		break;

		case IDM_VIEW_REDUCETABBAR :
		{
			_toReduceTabBar = !_toReduceTabBar;

			//Resize the  icon
			int iconSize = _toReduceTabBar?12:18;

			//Resize the tab height
			int tabHeight = _toReduceTabBar?20:25;
			TabCtrl_SetItemSize(_mainDocTab.getHSelf(), 45, tabHeight);
			TabCtrl_SetItemSize(_subDocTab.getHSelf(), 45, tabHeight);
			_docTabIconList.setIconSize(iconSize);

			//change the font
			int stockedFont = _toReduceTabBar?DEFAULT_GUI_FONT:SYSTEM_FONT;
			HFONT hf = (HFONT)::GetStockObject(stockedFont);

			if (hf)
			{
				::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
				::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
			}

			::SendMessage(_hSelf, WM_SIZE, 0, 0);
			break;
		}
		
		case IDM_VIEW_REFRESHTABAR :
		{
			::SendMessage(_hSelf, WM_SIZE, 0, 0);
			break;
		}
        case IDM_VIEW_LOCKTABBAR:
		{
			bool isDrag = TabBarPlus::doDragNDropOrNot();
            TabBarPlus::doDragNDrop(!isDrag);
			//checkMenuItem(IDM_VIEW_LOCKTABBAR, isDrag);
            break;
		}


		case IDM_VIEW_DRAWTABBAR_INACIVETAB:
		{
			TabBarPlus::setDrawInactiveTab(!TabBarPlus::drawInactiveTab());
			//TabBarPlus::setDrawInactiveTab(!TabBarPlus::drawInactiveTab(), _subDocTab.getHSelf());
			break;
		}
		case IDM_VIEW_DRAWTABBAR_TOPBAR:
		{
			TabBarPlus::setDrawTopBar(!TabBarPlus::drawTopBar());
			break;
		}

		case IDM_VIEW_DRAWTABBAR_CLOSEBOTTUN :
		{
			TabBarPlus::setDrawTabCloseButton(!TabBarPlus::drawTabCloseButton());

			// This part is just for updating (redraw) the tabs
			{
				int tabHeight = TabBarPlus::drawTabCloseButton()?21:20;
				TabCtrl_SetItemSize(_mainDocTab.getHSelf(), 45, tabHeight);
				TabCtrl_SetItemSize(_subDocTab.getHSelf(), 45, tabHeight);
			}
			::SendMessage(_hSelf, WM_SIZE, 0, 0);
			break;
		}

		case IDM_VIEW_DRAWTABBAR_DBCLK2CLOSE :
		{
			TabBarPlus::setDbClk2Close(!TabBarPlus::isDbClk2Close());
			break;
		}
		
		case IDM_VIEW_DRAWTABBAR_VERTICAL :
		{
			TabBarPlus::setVertical(!TabBarPlus::isVertical());
			::SendMessage(_hSelf, WM_SIZE, 0, 0);
			break;
		}
		
		case IDM_VIEW_DRAWTABBAR_MULTILINE :
		{
			TabBarPlus::setMultiLine(!TabBarPlus::isMultiLine());
			::SendMessage(_hSelf, WM_SIZE, 0, 0);		
			break;
		}

		case IDM_VIEW_POSTIT :
		{
			DWORD dwStyle = GetWindowLong(_hSelf, GWL_STYLE);
			bool isPostItOn = (WS_CAPTION & dwStyle) == 0;
			if (isPostItOn)
			{
				dwStyle |= (WS_CAPTION | WS_SIZEBOX);
				::SetWindowLong(_hSelf, GWL_STYLE, dwStyle);

				if (_beforePostIt.isStatusbarShown)
					::SendMessage(_hSelf, NPPM_HIDESTATUSBAR, 0, FALSE);
				if (_beforePostIt.isMenuShown)
					::SendMessage(_hSelf, NPPM_HIDEMENU, 0, FALSE);
				if (_beforePostIt.isToolbarShown)
					::SendMessage(_hSelf, NPPM_HIDETOOLBAR, 0, FALSE);
				if (_beforePostIt.isTabbarShown)
					::SendMessage(_hSelf, NPPM_HIDETABBAR, 0, FALSE);

				if (!_beforePostIt.isAlwaysOnTop)
					::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);

				_beforePostIt.isPostIt = false;
				
			}
			else // PostIt
			{
				NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
				// get current status before switch to postIt
				_beforePostIt.isPostIt = true;
				_beforePostIt.isSizable = true;
				_beforePostIt.isAlwaysOnTop = ::GetMenuState(_mainMenuHandle, IDM_VIEW_ALWAYSONTOP, MF_BYCOMMAND) == MF_CHECKED;
				_beforePostIt.isMenuShown = ::SendMessage(_hSelf, NPPM_ISMENUHIDDEN, 0, 0) != TRUE;
				_beforePostIt.isToolbarShown = ::SendMessage(_hSelf, NPPM_ISTOOLBARHIDDEN, 0, 0) != TRUE;
				_beforePostIt.isTabbarShown = ::SendMessage(_hSelf, NPPM_ISTABBARHIDDEN, 0, 0) != TRUE;
				_beforePostIt.isStatusbarShown = nppGUI._statusBarShow;

				// PostIt!
				dwStyle &= ~(WS_CAPTION | WS_SIZEBOX);
				::SetWindowLong(_hSelf, GWL_STYLE, dwStyle);

				if (nppGUI._statusBarShow)
					::SendMessage(_hSelf, NPPM_HIDESTATUSBAR, 0, TRUE);
				if (_beforePostIt.isMenuShown)
					::SendMessage(_hSelf, NPPM_HIDEMENU, 0, TRUE);
				
				if (_beforePostIt.isTabbarShown)
					::SendMessage(_hSelf, NPPM_HIDETABBAR, 0, TRUE);
				if (_beforePostIt.isToolbarShown)
					::SendMessage(_hSelf, NPPM_HIDETOOLBAR, 0, TRUE);

				if (!_beforePostIt.isAlwaysOnTop)
					::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_ALWAYSONTOP, 0);
			}
			
			//::SendMessage(_hSelf, WM_SIZE, 0, 0);
		}
		break;

		case IDM_VIEW_TAB_SPACE:
		{
			bool isChecked = !(::GetMenuState(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND) == MF_CHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_ALL_CHARACTERS, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND | (isChecked?MF_CHECKED:MF_UNCHECKED));
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, false);
			_pEditView->showEOL(false);
			_pEditView->showWSAndTab(isChecked);
			break;
		}
		case IDM_VIEW_EOL:
		{
			bool isChecked = !(::GetMenuState(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND) == MF_CHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND | (isChecked?MF_CHECKED:MF_UNCHECKED));
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_ALL_CHARACTERS, MF_BYCOMMAND | MF_UNCHECKED);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, false);
			_pEditView->showEOL(isChecked);
			_pEditView->showWSAndTab(false);
			break;
		}
		case IDM_VIEW_ALL_CHARACTERS:
		{
			bool isChecked = !(::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_EOL, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_TAB_SPACE, MF_BYCOMMAND | MF_UNCHECKED);
			::CheckMenuItem(_mainMenuHandle, IDM_VIEW_ALL_CHARACTERS, MF_BYCOMMAND | (isChecked?MF_CHECKED:MF_UNCHECKED));
			_pEditView->showInvisibleChars(isChecked);
			_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, isChecked);
			break;
		}

		case IDM_VIEW_INDENT_GUIDE:
		{
			_pEditView->showIndentGuideLine(!_pEditView->isShownIndentGuide());
            _toolBar.setCheck(IDM_VIEW_INDENT_GUIDE, _pEditView->isShownIndentGuide());
			checkMenuItem(IDM_VIEW_INDENT_GUIDE, _pEditView->isShownIndentGuide());
			break;
		}

		case IDM_VIEW_WRAP:
		{
			bool isWraped = !_pEditView->isWrap();
			_pEditView->wrap(isWraped);
            _toolBar.setCheck(IDM_VIEW_WRAP, isWraped);
			checkMenuItem(IDM_VIEW_WRAP, isWraped);
			break;
		}
		case IDM_VIEW_WRAP_SYMBOL:
		{
			_pEditView->showWrapSymbol(!_pEditView->isWrapSymbolVisible());
			checkMenuItem(IDM_VIEW_WRAP_SYMBOL, _pEditView->isWrapSymbolVisible());
			break;
		}

		case IDM_VIEW_HIDELINES:
		{
			_pEditView->hideLines();
			break;
		}

		case IDM_VIEW_ZOOMIN:
		{
			_pEditView->execute(SCI_ZOOMIN);
			break;
		}
		case IDM_VIEW_ZOOMOUT:
			_pEditView->execute(SCI_ZOOMOUT);
			break;

		case IDM_VIEW_ZOOMRESTORE:
			//Zoom factor of 0 points means default view
			_pEditView->execute(SCI_SETZOOM, 0);//_zoomOriginalValue);
			break;

		case IDM_VIEW_SYNSCROLLV:
		{
			_syncInfo._isSynScollV = !_syncInfo._isSynScollV;
			checkMenuItem(IDM_VIEW_SYNSCROLLV, _syncInfo._isSynScollV);
			_toolBar.setCheck(IDM_VIEW_SYNSCROLLV, _syncInfo._isSynScollV);

			if (_syncInfo._isSynScollV)
			{
				int mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
				int subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
				_syncInfo._line = mainCurrentLine - subCurrentLine;
			}
			
		}
		break;

		case IDM_VIEW_SYNSCROLLH:
		{
			_syncInfo._isSynScollH = !_syncInfo._isSynScollH;
			checkMenuItem(IDM_VIEW_SYNSCROLLH, _syncInfo._isSynScollH);
			_toolBar.setCheck(IDM_VIEW_SYNSCROLLH, _syncInfo._isSynScollH);

			if (_syncInfo._isSynScollH)
			{
				int mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
				int pixel = int(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
				int mainColumn = mxoffset/pixel;

				int sxoffset = _subEditView.execute(SCI_GETXOFFSET);
				pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
				int subColumn = sxoffset/pixel;
				_syncInfo._column = mainColumn - subColumn;
			}
		}
		break;

		case IDM_EXECUTE:
		{
			bool isFirstTime = !_runDlg.isCreated();
			_runDlg.doDialog(_isRTL);
			if (isFirstTime)
				changeDlgLang(_runDlg.getHSelf(), "Run");

			break;
		}

		case IDM_FORMAT_TODOS :
		case IDM_FORMAT_TOUNIX :
		case IDM_FORMAT_TOMAC :
		{
			Buffer * buf = _pEditView->getCurrentBuffer();
			
			int f = int((id == IDM_FORMAT_TODOS)?SC_EOL_CRLF:(id == IDM_FORMAT_TOUNIX)?SC_EOL_LF:SC_EOL_CR);

			buf->setFormat((formatType)f);
			_pEditView->execute(SCI_CONVERTEOLS, buf->getFormat());
			break;
		}

		case IDM_FORMAT_ANSI :
		case IDM_FORMAT_UTF_8 :	
		case IDM_FORMAT_UCS_2BE :
		case IDM_FORMAT_UCS_2LE :
		case IDM_FORMAT_AS_UTF_8 :
		{
			Buffer * buf = _pEditView->getCurrentBuffer();

			UniMode um;
			bool shoulBeDirty = true;
			switch (id)
			{
				case IDM_FORMAT_AS_UTF_8:
					shoulBeDirty = buf->getUnicodeMode() != uni8Bit;
					um = uniCookie;
					break;
				
				case IDM_FORMAT_UTF_8:
					um = uniUTF8;
					break;

				case IDM_FORMAT_UCS_2BE:
					um = uni16BE;
					break;

				case IDM_FORMAT_UCS_2LE:
					um = uni16LE;
					break;

				default : // IDM_FORMAT_ANSI
					shoulBeDirty = buf->getUnicodeMode() != uniCookie;
					um = uni8Bit;
			}

			if (buf->getUnicodeMode() != um)
			{
				buf->setUnicodeMode(um);
				if (shoulBeDirty)
					buf->setDirty(true);
			}
			break;
		}

		case IDM_FORMAT_CONV2_ANSI:
		case IDM_FORMAT_CONV2_AS_UTF_8:
		case IDM_FORMAT_CONV2_UTF_8:
		case IDM_FORMAT_CONV2_UCS_2BE: 
		case IDM_FORMAT_CONV2_UCS_2LE:
		{
			int idEncoding = -1;
			UniMode um = _pEditView->getCurrentBuffer()->getUnicodeMode();

			switch(id)
			{
				case IDM_FORMAT_CONV2_ANSI:
				{
					if (um == uni8Bit)
						return;
						
					idEncoding = IDM_FORMAT_ANSI;
					break;
				}
				case IDM_FORMAT_CONV2_AS_UTF_8:
				{
					idEncoding = IDM_FORMAT_AS_UTF_8;
					if (um == uniCookie)
						return;

					if (um != uni8Bit)
					{
						::SendMessage(_hSelf, WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}

					break;
				}
				case IDM_FORMAT_CONV2_UTF_8:
				{
					idEncoding = IDM_FORMAT_UTF_8;
					if (um == uniUTF8)
						return;

					if (um != uni8Bit)
					{
						::SendMessage(_hSelf, WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}
					break;
				}
		
				case IDM_FORMAT_CONV2_UCS_2BE:
				{
					idEncoding = IDM_FORMAT_UCS_2BE;
					if (um == uni16BE)
						return;

					if (um != uni8Bit)
					{
						::SendMessage(_hSelf, WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}
					break;
				}
		
				case IDM_FORMAT_CONV2_UCS_2LE:
				{
					idEncoding = IDM_FORMAT_UCS_2LE;
					if (um == uni16LE)
						return;
					if (um != uni8Bit)
					{
						::SendMessage(_hSelf, WM_COMMAND, idEncoding, 0);
						_pEditView->execute(SCI_EMPTYUNDOBUFFER);
						return;
					}
					break;
				}
			}

			if (idEncoding != -1)
			{
				// Save the current clipboard content
				::OpenClipboard(_hSelf);
				HANDLE clipboardData = ::GetClipboardData(CF_TEXT);
				int len = ::GlobalSize(clipboardData);
				LPVOID clipboardDataPtr = ::GlobalLock(clipboardData);

				HANDLE allocClipboardData = ::GlobalAlloc(GMEM_MOVEABLE, len);
				LPVOID clipboardData2 = ::GlobalLock(allocClipboardData);

				::memcpy(clipboardData2, clipboardDataPtr, len);
				::GlobalUnlock(clipboardData);	
				::GlobalUnlock(allocClipboardData);	
				::CloseClipboard();

				_pEditView->saveCurrentPos();

				// Cut all text
				int docLen = _pEditView->getCurrentDocLen();
				_pEditView->execute(SCI_COPYRANGE, 0, docLen);
				_pEditView->execute(SCI_CLEARALL);

				// Change to the proper buffer, save buffer status
				
				::SendMessage(_hSelf, WM_COMMAND, idEncoding, 0);

				// Paste the texte, restore buffer status
				_pEditView->execute(SCI_PASTE);
				_pEditView->restoreCurrentPos();

				// Restore the previous clipboard data
				::OpenClipboard(_hSelf);
				::EmptyClipboard(); 
				::SetClipboardData(CF_TEXT, clipboardData2);
				::CloseClipboard();

				//Do not free anything, EmptyClipboard does that
				_pEditView->execute(SCI_EMPTYUNDOBUFFER);
			}
			break;
		}

		case IDM_SETTING_TAB_REPLCESPACE:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			nppgui._tabReplacedBySpace = !nppgui._tabReplacedBySpace;
			_pEditView->execute(SCI_SETUSETABS, !nppgui._tabReplacedBySpace);
			//checkMenuItem(IDM_SETTING_TAB_REPLCESPACE, nppgui._tabReplacedBySpace);
			break;
		}

		case IDM_SETTING_TAB_SIZE:
		{
			ValueDlg tabSizeDlg;
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			tabSizeDlg.init(_hInst, _preference.getHSelf(), nppgui._tabSize, TEXT("Tab Size : "));
			POINT p;
			::GetCursorPos(&p);
			::ScreenToClient(_hParent, &p);
			int size = tabSizeDlg.doDialog(p, _isRTL);
			if (size != -1)
			{
				nppgui._tabSize = size;
				_pEditView->execute(SCI_SETTABWIDTH, nppgui._tabSize);
			}

			break;
		}

		case IDM_SETTING_AUTOCNBCHAR:
		{
			const int NB_MIN_CHAR = 1;
			const int NB_MAX_CHAR = 9;

			ValueDlg valDlg;
			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			valDlg.init(_hInst, _preference.getHSelf(), nppGUI._autocFromLen, TEXT("Nb char : "));
			POINT p;
			::GetCursorPos(&p);
			::ScreenToClient(_hParent, &p);
			int size = valDlg.doDialog(p, _isRTL);

			if (size != -1)
			{
				if (size > NB_MAX_CHAR)
					size = NB_MAX_CHAR;
				else if (size < NB_MIN_CHAR)
					size = NB_MIN_CHAR;
				
				nppGUI._autocFromLen = size;
			}
			break;
		}

		case IDM_SETTING_HISTORY_SIZE :
		{
			ValueDlg nbHistoryDlg;
			NppParameters *pNppParam = NppParameters::getInstance();
			nbHistoryDlg.init(_hInst, _preference.getHSelf(), pNppParam->getNbMaxFile(), TEXT("Max File : "));
			POINT p;
			::GetCursorPos(&p);
			::ScreenToClient(_hParent, &p);
			int size = nbHistoryDlg.doDialog(p, _isRTL);

			if (size != -1)
			{
				if (size > NB_MAX_LRF_FILE)
					size = NB_MAX_LRF_FILE;
				pNppParam->setNbMaxFile(size);
				_lastRecentFileList.setUserMaxNbLRF(size);
			}
			break;
		}

		case IDM_SETTING_FILEASSOCIATION_DLG :
		{
			RegExtDlg regExtDlg;
			regExtDlg.init(_hInst, _hSelf);
			regExtDlg.doDialog(_isRTL);
			break;
		}

		case IDM_SETTING_SHORTCUT_MAPPER :
		{
			ShortcutMapper shortcutMapper;
			shortcutMapper.init(_hInst, _hSelf);
			changeShortcutmapperLang(&shortcutMapper);
			shortcutMapper.doDialog(_isRTL);
			shortcutMapper.destroy();
			break;
		}

		case IDM_SETTING_PREFERECE :
		{
			bool isFirstTime = !_preference.isCreated();
			_preference.doDialog(_isRTL);
			
			if (isFirstTime)
			{
				changePrefereceDlgLang();
			}
			break;
		}

        case IDM_VIEW_GOTO_ANOTHER_VIEW:
            docGotoAnotherEditView(TransferMove);
			checkSyncState();
            break;

        case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
            docGotoAnotherEditView(TransferClone);
			checkSyncState();
            break;

        case IDM_VIEW_GOTO_NEW_INSTANCE :
            docOpenInNewInstance(TransferMove);
            break;

        case IDM_VIEW_LOAD_IN_NEW_INSTANCE:
            docOpenInNewInstance(TransferClone);
            break;

		case IDM_VIEW_SWITCHTO_OTHER_VIEW:
		{
			int view_to_focus;
			HWND wnd = GetFocus();
			if (_pEditView->getHSelf() == wnd)
			{
				view_to_focus = otherView();
				if (!viewVisible(view_to_focus)) view_to_focus = _activeView;
			}
			else
			{
				view_to_focus = currentView();
			}
			switchEditViewTo(view_to_focus);
			break;
		}

        case IDM_ABOUT:
		{
			bool isFirstTime = !_aboutDlg.isCreated();
            _aboutDlg.doDialog();
			if (isFirstTime && _nativeLangA)
			{
				const char *lang = (_nativeLangA->ToElement())->Attribute("name");

				if (_nativeLangEncoding == CP_BIG5)
				{
					char *authorName = "«J¤µ§^";
					HWND hItem = ::GetDlgItem(_aboutDlg.getHSelf(), IDC_AUTHOR_NAME);
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *authorNameW = wmc->char2wchar(authorName, CP_BIG5);
					::SetWindowText(hItem, authorNameW);
#else
					::SetWindowText(hItem, authorName);
#endif
				}
			}
			break;
		}

		case IDM_HELP :
		{
			TCHAR tmp[MAX_PATH];
			lstrcpy(tmp, _nppPath);
			::PathRemoveFileSpec(tmp);

			generic_string nppHelpPath = tmp;
			nppHelpPath += TEXT("\\NppHelp.chm");
			if (::PathFileExists(nppHelpPath.c_str()))
				::ShellExecute(NULL, TEXT("open"), nppHelpPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			else
			{
				generic_string msg = nppHelpPath;
				msg += TEXT("\rdoesn't exist. Please download it on Notepad++ site.");
				::MessageBox(_hSelf, msg.c_str(), TEXT("File does not exist"), MB_OK);
			}
		}
		break;

		case IDM_HOMESWEETHOME :
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://notepad-plus.sourceforge.net/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		case IDM_PROJECTPAGE :
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://sourceforge.net/projects/notepad-plus/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_ONLINEHELP:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://notepad-plus.sourceforge.net/uk/generalFAQ.php"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_WIKIFAQ:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://notepad-plus.wiki.sourceforge.net/FAQ"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		
		case IDM_FORUM:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("http://sourceforge.net/forum/?group_id=95717"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_PLUGINSHOME:
		{
			::ShellExecute(NULL, TEXT("open"), TEXT("https://sourceforge.net/projects/npp-plugins/"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_UPDATE_NPP :
		{
			generic_string updaterDir = pNppParam->getNppPath();
			updaterDir += TEXT("\\updater\\");
			generic_string updaterFullPath = updaterDir + TEXT("gup.exe");
			generic_string param = TEXT("-verbose -v");
			param += VERSION_VALUE;
			Process updater(updaterFullPath.c_str(), param.c_str(), updaterDir.c_str());
			updater.run();
			break;
		}

		case IDM_EDIT_AUTOCOMPLETE :
			showAutoComp();
			break;

		case IDM_EDIT_AUTOCOMPLETE_CURRENTFILE :
			autoCompFromCurrentFile();
			break;

		case IDM_EDIT_FUNCCALLTIP :
			showFunctionComp();
			break;

        case IDM_LANGSTYLE_CONFIG_DLG :
		{
			bool isFirstTime = !_configStyleDlg.isCreated();
			_configStyleDlg.doDialog(_isRTL);
			if (isFirstTime)
				changeConfigLang();
			break;
		}

        case IDM_LANG_C	:
        case IDM_LANG_CPP :
        case IDM_LANG_JAVA :
        case IDM_LANG_CS :
        case IDM_LANG_HTML :
        case IDM_LANG_XML :
        case IDM_LANG_JS :
        case IDM_LANG_PHP :
        case IDM_LANG_ASP :
        case IDM_LANG_CSS :
        case IDM_LANG_LUA :
        case IDM_LANG_PERL :
        case IDM_LANG_PYTHON :
        case IDM_LANG_PASCAL :
        case IDM_LANG_BATCH :
        case IDM_LANG_OBJC :
        case IDM_LANG_VB :
        case IDM_LANG_SQL :
        case IDM_LANG_ASCII :
        case IDM_LANG_TEXT :
        case IDM_LANG_RC :
        case IDM_LANG_MAKEFILE :
        case IDM_LANG_INI :
        case IDM_LANG_TEX :
        case IDM_LANG_FORTRAN :
        case IDM_LANG_SH :
        case IDM_LANG_FLASH :
		case IDM_LANG_NSIS :
		case IDM_LANG_TCL :
		case IDM_LANG_LISP :
		case IDM_LANG_SCHEME :
		case IDM_LANG_ASM :
		case IDM_LANG_DIFF :
		case IDM_LANG_PROPS :
		case IDM_LANG_PS:
		case IDM_LANG_RUBY:
		case IDM_LANG_SMALLTALK:
		case IDM_LANG_VHDL :
        case IDM_LANG_KIX :
        case IDM_LANG_CAML :
        case IDM_LANG_ADA :
        case IDM_LANG_VERILOG :
		case IDM_LANG_MATLAB :
		case IDM_LANG_HASKELL :
        case IDM_LANG_AU3 :
		case IDM_LANG_INNO :
		case IDM_LANG_CMAKE :
		case IDM_LANG_YAML :
		case IDM_LANG_USER :
		{
            setLanguage(id, menuID2LangType(id));
		}
        break;

        case IDC_PREV_DOC :
        case IDC_NEXT_DOC :
        {
			int nbDoc = viewVisible(MAIN_VIEW)?_mainDocTab.nbItem():0;
			nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;
			
			bool doTaskList = ((NppParameters::getInstance())->getNppGUI())._doTaskList;
			if (nbDoc > 1)
			{
				bool direction = (id == IDC_NEXT_DOC)?dirDown:dirUp;

				if (!doTaskList)
				{
					activateNextDoc(direction);
				}
				else
				{		
					TaskListDlg tld;
					HIMAGELIST hImgLst = _docTabIconList.getHandle();
					tld.init(_hInst, _hSelf, hImgLst, direction);
					tld.doDialog();
				}
			}
			_linkTriggered = true;
		}
        break;

		case IDM_OPEN_ALL_RECENT_FILE : {
			BufferID lastOne = BUFFER_INVALID;
			int size = _lastRecentFileList.getSize();
			for (int i = size - 1; i >= 0; i--)
			{
				BufferID test = doOpen(_lastRecentFileList.getIndex(i).c_str());
				if (test != BUFFER_INVALID)
					lastOne = test;
			}
			if (lastOne != BUFFER_INVALID) {
				switchToFile(lastOne);
			}
			break; }

		case IDM_CLEAN_RECENT_FILE_LIST :
			_lastRecentFileList.clear();
			break;

		case IDM_EDIT_RTL :
		case IDM_EDIT_LTR :
		{
			long exStyle = ::GetWindowLongPtr(_pEditView->getHSelf(), GWL_EXSTYLE);
			exStyle = (id == IDM_EDIT_RTL)?exStyle|WS_EX_LAYOUTRTL:exStyle&(~WS_EX_LAYOUTRTL);
			::SetWindowLongPtr(_pEditView->getHSelf(), GWL_EXSTYLE, exStyle);
			_pEditView->redraw();
		}
		break;

		case IDM_WINDOW_WINDOWS :
		{
			WindowsDlg _windowsDlg;
			_windowsDlg.init(_hInst, _hSelf, _pDocTab);
			
			TiXmlNodeA *dlgNode = NULL;
			if (_nativeLangA)
			{
				dlgNode = _nativeLangA->FirstChild("Dialog");
				if (dlgNode)
					dlgNode = searchDlgNode(dlgNode, "Window");
			}
			_windowsDlg.doDialog(dlgNode);
		}
		break;

		default :
			if (id > IDM_FILEMENU_LASTONE && id < (IDM_FILEMENU_LASTONE + _lastRecentFileList.getMaxNbLRF() + 1))
			{
				BufferID lastOpened = doOpen(_lastRecentFileList.getItem(id).c_str());
				if (lastOpened != BUFFER_INVALID) {
					switchToFile(lastOpened);
				}
			}
			else if ((id > IDM_LANG_USER) && (id < IDM_LANG_USER_LIMIT))
			{
				TCHAR langName[langNameLenMax];
				::GetMenuString(_mainMenuHandle, id, langName, langNameLenMax, MF_BYCOMMAND);
				_pEditView->getCurrentBuffer()->setLangType(L_USER, langName);
			}
			else if ((id >= IDM_LANG_EXTERNAL) && (id <= IDM_LANG_EXTERNAL_LIMIT))
			{
				setLanguage(id, (LangType)(id - IDM_LANG_EXTERNAL + L_EXTERNAL));
			}
			else if ((id >= ID_MACRO) && (id < ID_MACRO_LIMIT))
			{
				int i = id - ID_MACRO;
				vector<MacroShortcut> & theMacros = pNppParam->getMacroList();
				Macro macro = theMacros[i].getMacro();
				_pEditView->execute(SCI_BEGINUNDOACTION);

				for (Macro::iterator step = macro.begin(); step != macro.end(); step++)
					step->PlayBack(this, _pEditView);
				
				_pEditView->execute(SCI_ENDUNDOACTION);
				
			}
			else if ((id >= ID_USER_CMD) && (id < ID_USER_CMD_LIMIT))
			{
				int i = id - ID_USER_CMD;
				vector<UserCommand> & theUserCommands = pNppParam->getUserCommandList();
				UserCommand ucmd = theUserCommands[i];

				Command cmd(ucmd.getCmd());
				cmd.run(_hSelf);
			}
			else if ((id >= ID_PLUGINS_CMD) && (id < ID_PLUGINS_CMD_LIMIT))
			{
				int i = id - ID_PLUGINS_CMD;
				_pluginsManager.runPluginCommand(i);
			}
			else if ((id >= IDM_WINDOW_MRU_FIRST) && (id <= IDM_WINDOW_MRU_LIMIT))
			{
				activateDoc(id-IDM_WINDOW_MRU_FIRST);				
			}
	}
	
	if (_recordingMacro) 
		switch (id)
		{
			case IDM_FILE_NEW :
			case IDM_FILE_CLOSE :
			case IDM_FILE_CLOSEALL :
			case IDM_FILE_CLOSEALL_BUT_CURRENT :
			case IDM_FILE_SAVE :
			case IDM_FILE_SAVEALL :
			case IDM_EDIT_UNDO:
			case IDM_EDIT_REDO:
			case IDM_EDIT_CUT:
			case IDM_EDIT_COPY:
			//case IDM_EDIT_PASTE:
			case IDM_EDIT_DELETE:
			case IDM_SEARCH_FINDNEXT :
			case IDM_SEARCH_FINDPREV :
			case IDM_SEARCH_MARKALL :
			case IDM_SEARCH_UNMARKALL :
			case IDM_SEARCH_GOTOMATCHINGBRACE :
			case IDM_SEARCH_TOGGLE_BOOKMARK :
			case IDM_SEARCH_NEXT_BOOKMARK:
			case IDM_SEARCH_PREV_BOOKMARK:
			case IDM_SEARCH_CLEAR_BOOKMARKS:
			case IDM_EDIT_SELECTALL:
			case IDM_EDIT_INS_TAB:
			case IDM_EDIT_RMV_TAB:
			case IDM_EDIT_DUP_LINE:
			case IDM_EDIT_TRANSPOSE_LINE:
			case IDM_EDIT_SPLIT_LINES:
			case IDM_EDIT_JOIN_LINES:
			case IDM_EDIT_LINE_UP:
			case IDM_EDIT_LINE_DOWN:
			case IDM_EDIT_UPPERCASE:
			case IDM_EDIT_LOWERCASE:
			case IDM_EDIT_BLOCK_COMMENT:
			case IDM_EDIT_BLOCK_COMMENT_SET:
			case IDM_EDIT_BLOCK_UNCOMMENT:
			case IDM_EDIT_STREAM_COMMENT:
			case IDM_EDIT_TRIMTRAILING:
			case IDM_EDIT_SETREADONLY :
			case IDM_EDIT_FULLPATHTOCLIP :
			case IDM_EDIT_FILENAMETOCLIP :
			case IDM_EDIT_CURRENTDIRTOCLIP :
			case IDM_EDIT_CLEARREADONLY :
			case IDM_EDIT_RTL :
			case IDM_EDIT_LTR :
			case IDM_VIEW_FULLSCREENTOGGLE :
			case IDM_VIEW_ALWAYSONTOP :
			case IDM_VIEW_WRAP :
			case IDM_VIEW_FOLD_CURRENT :
			case IDM_VIEW_UNFOLD_CURRENT :
			case IDM_VIEW_TOGGLE_FOLDALL:
			case IDM_VIEW_TOGGLE_UNFOLDALL:
			case IDM_VIEW_FOLD_1:
			case IDM_VIEW_FOLD_2:
			case IDM_VIEW_FOLD_3:
			case IDM_VIEW_FOLD_4:
			case IDM_VIEW_FOLD_5:
			case IDM_VIEW_FOLD_6:
			case IDM_VIEW_FOLD_7:
			case IDM_VIEW_FOLD_8:
			case IDM_VIEW_UNFOLD_1:
			case IDM_VIEW_UNFOLD_2:
			case IDM_VIEW_UNFOLD_3:
			case IDM_VIEW_UNFOLD_4:
			case IDM_VIEW_UNFOLD_5:
			case IDM_VIEW_UNFOLD_6:
			case IDM_VIEW_UNFOLD_7:
			case IDM_VIEW_UNFOLD_8:
			case IDM_VIEW_GOTO_ANOTHER_VIEW:
			case IDM_VIEW_SYNSCROLLV:
			case IDM_VIEW_SYNSCROLLH:
			case IDC_PREV_DOC :
			case IDC_NEXT_DOC :
				_macro.push_back(recordedMacroStep(id));
				break;
		}

}

void Notepad_plus::setLanguage(int id, LangType langType) {
	//Add logic to prevent changing a language when a document is shared between two views
	//If so, release one document
	bool reset = false;
	Document prev = 0;
	if (bothActive()) {
		if (_mainEditView.getCurrentBufferID() == _subEditView.getCurrentBufferID()) {
			reset = true;
			_subEditView.saveCurrentPos();
			prev = _subEditView.execute(SCI_GETDOCPOINTER);
			_subEditView.execute(SCI_SETDOCPOINTER, 0, 0);
		}
	}
	if (reset) {
		_mainEditView.getCurrentBuffer()->setLangType(langType);
	} else {
		_pEditView->getCurrentBuffer()->setLangType(langType);
	}

	if (reset) {
		_subEditView.execute(SCI_SETDOCPOINTER, 0, prev);
		_subEditView.restoreCurrentPos();
	}
};

enum LangType Notepad_plus::menuID2LangType(int cmdID)
{
	switch (cmdID)
	{
        case IDM_LANG_C	:
            return L_C;
        case IDM_LANG_CPP :
            return L_CPP;
        case IDM_LANG_JAVA :
            return L_JAVA;
        case IDM_LANG_CS :
            return L_CS;
        case IDM_LANG_HTML :
            return L_HTML;
        case IDM_LANG_XML :
            return L_XML;
        case IDM_LANG_JS :
            return L_JS;
        case IDM_LANG_PHP :
            return L_PHP;
        case IDM_LANG_ASP :
            return L_ASP;
        case IDM_LANG_CSS :
            return L_CSS;
        case IDM_LANG_LUA :
            return L_LUA;
        case IDM_LANG_PERL :
            return L_PERL;
        case IDM_LANG_PYTHON :
            return L_PYTHON;
        case IDM_LANG_PASCAL :
            return L_PASCAL;
        case IDM_LANG_BATCH :
            return L_BATCH;
        case IDM_LANG_OBJC :
            return L_OBJC;
        case IDM_LANG_VB :
            return L_VB;
        case IDM_LANG_SQL :
            return L_SQL;
        case IDM_LANG_ASCII :
            return L_NFO;
        case IDM_LANG_TEXT :
            return L_TXT;
        case IDM_LANG_RC :
            return L_RC;
        case IDM_LANG_MAKEFILE :
            return L_MAKEFILE;
        case IDM_LANG_INI :
            return L_INI;
        case IDM_LANG_TEX :
            return L_TEX;
        case IDM_LANG_FORTRAN :
            return L_FORTRAN;
        case IDM_LANG_SH :
            return L_BASH;
        case IDM_LANG_FLASH :
            return L_FLASH;
		case IDM_LANG_NSIS :
            return L_NSIS;
		case IDM_LANG_TCL :
            return L_TCL;
		case IDM_LANG_LISP :
			return L_LISP;
		case IDM_LANG_SCHEME :
			return L_SCHEME;
		case IDM_LANG_ASM :
            return L_ASM;
		case IDM_LANG_DIFF :
            return L_DIFF;
		case IDM_LANG_PROPS :
            return L_PROPS;
		case IDM_LANG_PS:
            return L_PS;
		case IDM_LANG_RUBY:
            return L_RUBY;
		case IDM_LANG_SMALLTALK:
            return L_SMALLTALK;
		case IDM_LANG_VHDL :
            return L_VHDL;
        case IDM_LANG_KIX :
            return L_KIX;
        case IDM_LANG_CAML :
            return L_CAML;
        case IDM_LANG_ADA :
            return L_ADA;
        case IDM_LANG_VERILOG :
            return L_VERILOG;
		case IDM_LANG_MATLAB :
            return L_MATLAB;
		case IDM_LANG_HASKELL :
            return L_HASKELL;
        case IDM_LANG_AU3 :
            return L_AU3;
		case IDM_LANG_INNO :
            return L_INNO;
		case IDM_LANG_CMAKE :
            return L_CMAKE; 
		case IDM_LANG_YAML :
			return L_YAML; 
		case IDM_LANG_USER :
            return L_USER;
		default: {
			if (cmdID >= IDM_LANG_USER && cmdID <= IDM_LANG_USER_LIMIT) {
				return L_USER;
			}
			break; }
	}
	return L_EXTERNAL;
}

void Notepad_plus::setTitle()
{
	const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();
	//Get the buffer
	Buffer * buf = _pEditView->getCurrentBuffer();

	generic_string result = TEXT("");
	if (buf->isDirty())
	{
		result += TEXT("*");
	}

	if (nppGUI._shortTitlebar)
	{
		result += buf->getFileName();
	}
	else
	{
		result += buf->getFullPathName();
	}
	result += TEXT(" - ");
	result += _className;
	//::SetWindowText(_hSelf, title);
	::SendMessage(_hSelf, WM_SETTEXT, 0, (LPARAM)result.c_str());
	
}

void Notepad_plus::activateNextDoc(bool direction) 
{
	int nbDoc = _pDocTab->nbItem();

    int curIndex = _pDocTab->getCurrentTabIndex();
    curIndex += (direction == dirUp)?-1:1;

	if (curIndex >= nbDoc)
	{
		if (viewVisible(otherView()))
			switchEditViewTo(otherView());
		curIndex = 0;
	}
	else if (curIndex < 0)
	{
		if (viewVisible(otherView()))
		{
			switchEditViewTo(otherView());
			nbDoc = _pDocTab->nbItem();
		}
		curIndex = nbDoc - 1;
	}

	BufferID id = _pDocTab->getBufferByIndex(curIndex);
	activateBuffer(id, currentView());
}

void Notepad_plus::activateDoc(int pos)
{
	int nbDoc = _pDocTab->nbItem();
	if (pos == _pDocTab->getCurrentTabIndex())
	{
		Buffer * buf = _pEditView->getCurrentBuffer();
		buf->increaseRecentTag();
		return;
	}

	if (pos >= 0 && pos < nbDoc)
	{
		BufferID id = _pDocTab->getBufferByIndex(pos);
		activateBuffer(id, currentView());
	}
}

void Notepad_plus::updateStatusBar() 
{
	Buffer * buf = _pEditView->getCurrentBuffer();
    TCHAR strLnCol[64];
	wsprintf(strLnCol, TEXT("Ln : %d    Col : %d    Sel : %d"),\
        (_pEditView->getCurrentLineNumber() + 1), \
		(_pEditView->getCurrentColumnNumber() + 1),\
		(_pEditView->getSelectedByteNumber()));

    _statusBar.setText(strLnCol, STATUSBAR_CUR_POS);

	TCHAR strDonLen[64];
	wsprintf(strDonLen, TEXT("nb char : %d"), _pEditView->getCurrentDocLen());
	_statusBar.setText(strDonLen, STATUSBAR_DOC_SIZE);
    _statusBar.setText(_pEditView->execute(SCI_GETOVERTYPE) ? TEXT("OVR") : TEXT("INS"), STATUSBAR_TYPING_MODE);
}


void Notepad_plus::dropFiles(HDROP hdrop) 
{
	if (hdrop)
	{
		// Determinate in which view the file(s) is (are) dropped
		POINT p;
		::DragQueryPoint(hdrop, &p);
		HWND hWin = ::RealChildWindowFromPoint(_hSelf, p);
		if (!hWin) return;

		if ((_mainEditView.getHSelf() == hWin) || (_mainDocTab.getHSelf() == hWin))
			switchEditViewTo(MAIN_VIEW);
		else if ((_subEditView.getHSelf() == hWin) || (_subDocTab.getHSelf() == hWin))
			switchEditViewTo(SUB_VIEW);
		else
		{
			::SendMessage(hWin, WM_DROPFILES, (WPARAM)hdrop, 0);
			return;
		}

		int filesDropped = ::DragQueryFile(hdrop, 0xffffffff, NULL, 0);
		BufferID lastOpened = BUFFER_INVALID;
		for (int i = 0 ; i < filesDropped ; ++i)
		{
			TCHAR pathDropped[MAX_PATH];
			::DragQueryFile(hdrop, i, pathDropped, MAX_PATH);
			BufferID test = doOpen(pathDropped);
			if (test != BUFFER_INVALID)
				lastOpened = test;
            //setLangStatus(_pEditView->getCurrentDocType());
		}
		if (lastOpened != BUFFER_INVALID) {
			switchToFile(lastOpened);
		}
		::DragFinish(hdrop);
		// Put Notepad_plus to forefront
		// May not work for Win2k, but OK for lower versions
		// Note: how to drop a file to an iconic window?
		// Actually, it is the Send To command that generates a drop.
		if (::IsIconic(_hSelf))
		{
			::ShowWindow(_hSelf, SW_RESTORE);
		}
		::SetForegroundWindow(_hSelf);
	}
}

void Notepad_plus::checkModifiedDocument()
{
	//this will trigger buffer updates. If the status changes, Notepad++ will be informed and can do its magic
	MainFileManager->checkFilesystemChanges();
}

void Notepad_plus::getMainClientRect(RECT &rc) const
{
    getClientRect(rc);
	rc.top += _rebarTop.getHeight();
	rc.bottom -= rc.top + _rebarBottom.getHeight() + _statusBar.getHeight();
}

void Notepad_plus::showView(int whichOne) {
	if (viewVisible(whichOne))	//no use making visible view visible
		return;

	if (_mainWindowStatus & WindowUserActive) {
		 _pMainSplitter->setWin0(&_subSplitter);
		 _pMainWindow = _pMainSplitter;
	} else {
		_pMainWindow = &_subSplitter;
	}

	if (whichOne == MAIN_VIEW) {
		_mainEditView.display(true);
		_mainDocTab.display(true);
	} else if (whichOne == SUB_VIEW) {
		_subEditView.display(true);
		_subDocTab.display(true);
	}
	_pMainWindow->display(true);

	_mainWindowStatus |= (whichOne==MAIN_VIEW)?WindowMainActive:WindowSubActive;

	//Send sizing info to make windows fit
	::SendMessage(_hSelf, WM_SIZE, 0, 0);
}

bool Notepad_plus::viewVisible(int whichOne) {
	int viewToCheck = (whichOne == SUB_VIEW?WindowSubActive:WindowMainActive);
	return (_mainWindowStatus & viewToCheck) != 0;
}

void Notepad_plus::hideCurrentView()
{
	hideView(currentView());
}

void Notepad_plus::hideView(int whichOne)
{
	if (!(bothActive()))	//cannot close if not both views visible
		return;

	Window * windowToSet = (whichOne == MAIN_VIEW)?&_subDocTab:&_mainDocTab;
	if (_mainWindowStatus & WindowUserActive)
	{
		_pMainSplitter->setWin0(windowToSet);
	}
	else // otherwise the main window is the spltter container that we just created
		_pMainWindow = windowToSet;
	    
	_subSplitter.display(false);	//hide splitter
	//hide scintilla and doctab
	if (whichOne == MAIN_VIEW) {
		_mainEditView.display(false);
		_mainDocTab.display(false);
	} else if (whichOne == SUB_VIEW) {
		_subEditView.display(false);
		_subDocTab.display(false);
	}

	// resize the main window
	::SendMessage(_hSelf, WM_SIZE, 0, 0);

	switchEditViewTo(otherFromView(whichOne));
	int viewToDisable = (whichOne == SUB_VIEW?WindowSubActive:WindowMainActive);
	_mainWindowStatus &= ~viewToDisable;
}

bool Notepad_plus::reloadLang() 
{
	NppParameters *pNppParam = NppParameters::getInstance();

	if (!pNppParam->reloadLang())
	{
		return false;
	}

	TiXmlDocumentA *nativeLangDocRootA = pNppParam->getNativeLangA();
	if (!nativeLangDocRootA)
	{
		return false;
	}
	_nativeLangA =  nativeLangDocRootA->FirstChild("NotepadPlus");
	if (!_nativeLangA)
	{
		return false;
	}
	_nativeLangA = _nativeLangA->FirstChild("Native-Langue");
	if (!_nativeLangA)
	{
		return false;
	}
	TiXmlElementA *element = _nativeLangA->ToElement();
	const char *rtl = element->Attribute("RTL");
	if (rtl)
		_isRTL = (strcmp(rtl, "yes") == 0);

	// get encoding
	TiXmlDeclarationA *declaration =  _nativeLangA->GetDocument()->FirstChild()->ToDeclaration();
	if (declaration)
	{
		const char * encodingStr = declaration->Encoding();
		_nativeLangEncoding = getCpFromStringValue(encodingStr);
	}
	
	pNppParam->reloadContextMenuFromXmlTree(_mainMenuHandle);

	generic_string pluginsTrans, windowTrans;
	changeMenuLang(pluginsTrans, windowTrans);

	int indexWindow = ::GetMenuItemCount(_mainMenuHandle) - 3;

	if (_pluginsManager.hasPlugins() && pluginsTrans != TEXT(""))
	{
		::ModifyMenu(_mainMenuHandle, indexWindow - 1, MF_BYPOSITION, 0, pluginsTrans.c_str());
	}
	
	if (windowTrans != TEXT(""))
	{
		::ModifyMenu(_mainMenuHandle, indexWindow, MF_BYPOSITION, 0, windowTrans.c_str());
		windowTrans += TEXT("...");
		::ModifyMenu(_mainMenuHandle, IDM_WINDOW_WINDOWS, MF_BYCOMMAND, IDM_WINDOW_WINDOWS, windowTrans.c_str());
	}
	// Update scintilla context menu strings
	vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
	size_t len = tmp.size();
	TCHAR menuName[64];
	for (size_t i = 0 ; i < len ; i++)
	{
		if (tmp[i]._itemName == TEXT(""))
		{
			::GetMenuString(_mainMenuHandle, tmp[i]._cmdID, menuName, 64, MF_BYCOMMAND);
			tmp[i]._itemName = purgeMenuItemString(menuName);
		}
	}
	
	vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
	len = shortcuts.size();

	for(size_t i = 0; i < len; i++) 
	{
		CommandShortcut & csc = shortcuts[i];
		::GetMenuString(_mainMenuHandle, csc.getID(), menuName, 64, MF_BYCOMMAND);
		csc.setName(purgeMenuItemString(menuName, true).c_str());
	}
	_accelerator.updateFullMenu();

	_scintaccelerator.updateKeys();


	if (_tabPopupMenu.isCreated())
	{
		changeLangTabContextMenu();
	}
	if (_tabPopupDropMenu.isCreated())
	{
		changeLangTabDrapContextMenu();
	}

	if (_preference.isCreated())
	{
		changePrefereceDlgLang();
	}

	if (_configStyleDlg.isCreated())
	{
		changeConfigLang();
	}

	if (_findReplaceDlg.isCreated())
	{
		changeFindReplaceDlgLang();
	}

	if (_goToLineDlg.isCreated())
	{
		changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
	}

	if (_runDlg.isCreated())
	{
		changeDlgLang(_runDlg.getHSelf(), "Run");
	}

	if (_runMacroDlg.isCreated())
	{
		changeDlgLang(_runMacroDlg.getHSelf(), "MultiMacro");
	}

	if (_goToLineDlg.isCreated())
	{
		changeDlgLang(_goToLineDlg.getHSelf(), "GoToLine");
	}

	if (_colEditorDlg.isCreated())
	{
		changeDlgLang(_colEditorDlg.getHSelf(), "ColumnEditor");
	}

	UserDefineDialog *udd = _pEditView->getUserDefineDlg();
	if (udd->isCreated())
	{
		changeUserDefineLang();
	}

	return true;
}

bool Notepad_plus::canHideView(int whichOne)
{
	if (!viewVisible(whichOne))
		return false;	//cannot hide hidden view
	if (!bothActive())
		return false;	//cannot hide only window
	DocTabView * tabToCheck = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	Buffer * buf = MainFileManager->getBufferByID(tabToCheck->getBufferByIndex(0));
	bool canHide = ((tabToCheck->nbItem() == 1) && !buf->isDirty() && buf->isUntitled());
	return canHide;
}

void Notepad_plus::loadBufferIntoView(BufferID id, int whichOne, bool dontClose) {
	DocTabView * tabToOpen = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	ScintillaEditView * viewToOpen = (whichOne == MAIN_VIEW)?&_mainEditView:&_subEditView;

	//check if buffer exists
	int index = tabToOpen->getIndexByBuffer(id);
	if (index != -1)	//already open, done
		return;

	BufferID idToClose = BUFFER_INVALID;
	//Check if the tab has a single clean buffer. Close it if so
	if (!dontClose && tabToOpen->nbItem() == 1) {
		idToClose = tabToOpen->getBufferByIndex(0);
		Buffer * buf = MainFileManager->getBufferByID(idToClose);
		if (buf->isDirty() || !buf->isUntitled()) {
			idToClose = BUFFER_INVALID;
		}
	}

	MainFileManager->addBufferReference(id, viewToOpen);

	if (idToClose != BUFFER_INVALID) {	//close clean doc. Use special logic to prevent flicker of tab showing then hiding
		tabToOpen->setBuffer(0, id);	//index 0 since only one open
		activateBuffer(id, whichOne);	//activate. DocTab already activated but not a problem
		MainFileManager->closeBuffer(idToClose, viewToOpen);	//delete the buffer
	} else {
		tabToOpen->addBuffer(id);
	}
}

void Notepad_plus::removeBufferFromView(BufferID id, int whichOne) {
	DocTabView * tabToClose = (whichOne == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
	ScintillaEditView * viewToClose = (whichOne == MAIN_VIEW)?&_mainEditView:&_subEditView;

	//check if buffer exists
	int index = tabToClose->getIndexByBuffer(id);
	if (index == -1)	//doesnt exist, done
		return;
	
	Buffer * buf = MainFileManager->getBufferByID(id);
	
	//Cannot close doc if last and clean
	if (tabToClose->nbItem() == 1) {
		if (!buf->isDirty() && buf->isUntitled()) {
			return;	//done
		}
	}

	int active = tabToClose->getCurrentTabIndex();
	if (active == index) {	//need an alternative (close real doc, put empty one back
		if (tabToClose->nbItem() == 1) {	//need alternative doc, add new one. Use special logic to prevent flicker of adding new tab then closing other	
			BufferID newID = MainFileManager->newEmptyDocument();
			MainFileManager->addBufferReference(newID, viewToClose);
			tabToClose->setBuffer(0, newID);	//can safely use id 0, last (only) tab open
			activateBuffer(newID, whichOne);	//activate. DocTab already activated but not a problem
		} else {
			int toActivate = 0;
			//activate next doc, otherwise prev if not possible
			if (active == tabToClose->nbItem() - 1) {	//prev
				toActivate = active - 1;
			} else {
				toActivate = active;	//activate the 'active' index. Since we remove the tab first, the indices shift (on the right side)
			}
			tabToClose->deletItemAt(index);	//delete first
			activateBuffer(tabToClose->getBufferByIndex(toActivate), whichOne);	//then activate. The prevent jumpy tab behaviour
		}
	} else {
		tabToClose->deletItemAt(index);
	}

	MainFileManager->closeBuffer(id, viewToClose);
}

int Notepad_plus::switchEditViewTo(int gid) 
{
	if (currentView() == gid) {	//make sure focus is ok, then leave
		_pEditView->getFocus();	//set the focus
		return gid;
	}
	if (!viewVisible(gid))
		return currentView();	//cannot activate invisible view
	int oldView = currentView();
	int newView = otherView();

	_activeView = newView;
	//Good old switcheroo
	DocTabView * tempTab = _pDocTab;
	_pDocTab = _pNonDocTab;
	_pNonDocTab = tempTab;
	ScintillaEditView * tempView = _pEditView;
	_pEditView = _pNonEditView;
	_pNonEditView = tempView;

	_pEditView->beSwitched();
    _pEditView->getFocus();	//set the focus

	notifyBufferActivated(_pEditView->getCurrentBufferID(), currentView());
	return oldView;
}

void Notepad_plus::dockUserDlg()
{
    if (!_pMainSplitter)
    {
        _pMainSplitter = new SplitterContainer;
        _pMainSplitter->init(_hInst, _hSelf);

        Window *pWindow;
		if (_mainWindowStatus & (WindowMainActive | WindowSubActive))
            pWindow = &_subSplitter;
        else
            pWindow = _pDocTab;

        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, RIGHT_FIX, 45);
    }

    if (bothActive())
        _pMainSplitter->setWin0(&_subSplitter);
    else 
        _pMainSplitter->setWin0(_pDocTab);

    _pMainSplitter->display();

    _mainWindowStatus |= WindowUserActive;
    _pMainWindow = _pMainSplitter;

	::SendMessage(_hSelf, WM_SIZE, 0, 0);
}

void Notepad_plus::undockUserDlg()
{
    // a cause de surchargement de "display"
    ::ShowWindow(_pMainSplitter->getHSelf(), SW_HIDE);

    if (bothActive())
        _pMainWindow = &_subSplitter;
    else
        _pMainWindow = _pDocTab;
    
    ::SendMessage(_hSelf, WM_SIZE, 0, 0);

    _mainWindowStatus &= ~WindowUserActive;
    (ScintillaEditView::getUserDefineDlg())->display(); 
}

void Notepad_plus::docOpenInNewInstance(FileTransferMode mode)
{
	BufferID bufferID = _pEditView->getCurrentBufferID();
	Buffer * buf = MainFileManager->getBufferByID(bufferID);
	if (buf->isUntitled() || buf->isDirty())
		return;

	TCHAR nppName[MAX_PATH];
	::GetModuleFileName(NULL, nppName, MAX_PATH);
	std::generic_string command = TEXT("\"");
	command += nppName;
	command += TEXT("\"");

	command += TEXT(" \"$(FULL_CURRENT_PATH)\" -multiInst -nosession");

	Command cmd(command);
	cmd.run(_hSelf);
	if (mode == TransferMove)
		doClose(bufferID, currentView());
}

void Notepad_plus::docGotoAnotherEditView(FileTransferMode mode)
{
	//First put the doc in the other view if not present (if it is, activate it).
	//Then if needed close in the original tab
	BufferID current = _pEditView->getCurrentBufferID();
	int viewToGo = otherView();
	int indexFound = _pNonDocTab->getIndexByBuffer(current);
	if (indexFound != -1)	//activate it
	{
		activateBuffer(current, otherView());
	}
	else	//open the document, also copying the position
	{
		loadBufferIntoView(current, viewToGo);
		Buffer * buf = MainFileManager->getBufferByID(current);
		_pEditView->saveCurrentPos();	//allow copying of position
		buf->setPosition(buf->getPosition(_pEditView), _pNonEditView);
		_pNonEditView->restoreCurrentPos();	//set position
		activateBuffer(current, otherView());
	}

	//Open the view if it was hidden
	int viewToOpen = (viewToGo == SUB_VIEW?WindowSubActive:WindowMainActive);
	if (!(_mainWindowStatus & viewToOpen)) {
		showView(viewToGo);
	}

	//Close the document if we transfered the document instead of cloning it
	if (mode == TransferMove) 
	{
		//just close the activate document, since thats the one we moved (no search)
		doClose(_pEditView->getCurrentBufferID(), currentView());
	} // else it was cone, so leave it

	//Activate the other view since thats where the document went
	switchEditViewTo(viewToGo);

	//_linkTriggered = true;
}

bool Notepad_plus::activateBuffer(BufferID id, int whichOne)
{
	//scnN.nmhdr.code = NPPN_DOCSWITCHINGOFF;		//superseeded by NPPN_BUFFERACTIVATED

	Buffer * pBuf = MainFileManager->getBufferByID(id);
	bool reload = pBuf->getNeedReload();
	if (reload)
	{
		MainFileManager->reloadBuffer(id);
		pBuf->setNeedReload(false);
	}
	if (whichOne == MAIN_VIEW)
	{
		if (_mainDocTab.activateBuffer(id))	//only activate if possible
			_mainEditView.activateBuffer(id);
		else
			return false;
	}
	else
	{
		if (_subDocTab.activateBuffer(id))
			_subEditView.activateBuffer(id);
		else
			return false;
	}

	if (reload) 
	{
		performPostReload(whichOne);
	}
	notifyBufferActivated(id, whichOne);

	//scnN.nmhdr.code = NPPN_DOCSWITCHINGIN;		//superseeded by NPPN_BUFFERACTIVATED
	return true;
}

void Notepad_plus::performPostReload(int whichOne) {
	NppParameters *pNppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParam->getNppGUI();
	bool toEnd = (nppGUI._fileAutoDetection == cdAutoUpdateGo2end) || (nppGUI._fileAutoDetection == cdGo2end);
	if (!toEnd)
		return;
	if (whichOne == MAIN_VIEW) {
		_mainEditView.execute(SCI_GOTOLINE, _mainEditView.execute(SCI_GETLINECOUNT) -1);
	} else {
		_subEditView.execute(SCI_GOTOLINE, _subEditView.execute(SCI_GETLINECOUNT) -1);
	}	
}

void Notepad_plus::bookmarkNext(bool forwardScan) 
{
	int lineno = _pEditView->getCurrentLineNumber();
	int sci_marker = SCI_MARKERNEXT;
	int lineStart = lineno + 1;	//Scan starting from next line
	int lineRetry = 0;				//If not found, try from the beginning
	if (!forwardScan) 
    {
		lineStart = lineno - 1;		//Scan starting from previous line
		lineRetry = int(_pEditView->execute(SCI_GETLINECOUNT));	//If not found, try from the end
		sci_marker = SCI_MARKERPREVIOUS;
	}
	int nextLine = int(_pEditView->execute(sci_marker, lineStart, 1 << MARK_BOOKMARK));
	if (nextLine < 0)
		nextLine = int(_pEditView->execute(sci_marker, lineRetry, 1 << MARK_BOOKMARK));

	if (nextLine < 0)
		return;

    _pEditView->execute(SCI_ENSUREVISIBLEENFORCEPOLICY, nextLine);
	_pEditView->execute(SCI_GOTOLINE, nextLine);
}

void Notepad_plus::dynamicCheckMenuAndTB() const 
{
	// Visibility of 3 margins
    checkMenuItem(IDM_VIEW_LINENUMBER, _pEditView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER));
    checkMenuItem(IDM_VIEW_SYMBOLMARGIN, _pEditView->hasMarginShowed(ScintillaEditView::_SC_MARGE_SYBOLE));
    checkMenuItem(IDM_VIEW_FOLDERMAGIN, _pEditView->hasMarginShowed(ScintillaEditView::_SC_MARGE_FOLDER));

	// Folder margin style
	checkFolderMarginStyleMenu(getFolderMaginStyleIDFrom(_pEditView->getFolderStyle()));

	// Visibility of invisible characters
	bool wsTabShow = _pEditView->isInvisibleCharsShown();
	bool eolShow = _pEditView->isEolVisible();

	bool onlyWS = false;
	bool onlyEOL = false;
	bool bothWSEOL = false;
	if (wsTabShow)
	{
		if (eolShow)
		{
			bothWSEOL = true;
		}
		else
		{
			onlyWS = true;
		}
	}
	else if (eolShow)
	{
		onlyEOL = true;
	}
	
	checkMenuItem(IDM_VIEW_TAB_SPACE, onlyWS);
	checkMenuItem(IDM_VIEW_EOL, onlyEOL);
	checkMenuItem(IDM_VIEW_ALL_CHARACTERS, bothWSEOL);
	_toolBar.setCheck(IDM_VIEW_ALL_CHARACTERS, bothWSEOL);

	// Visibility of the indentation guide line 
	bool b = _pEditView->isShownIndentGuide();
	checkMenuItem(IDM_VIEW_INDENT_GUIDE, b);
	_toolBar.setCheck(IDM_VIEW_INDENT_GUIDE, b);

	// Edge Line
	int mode = int(_pEditView->execute(SCI_GETEDGEMODE));
	checkMenuItem(IDM_VIEW_EDGEBACKGROUND, (MF_BYCOMMAND | ((mode == EDGE_NONE)||(mode == EDGE_LINE))?MF_UNCHECKED:MF_CHECKED) != 0);
	checkMenuItem(IDM_VIEW_EDGELINE, (MF_BYCOMMAND | ((mode == EDGE_NONE)||(mode == EDGE_BACKGROUND))?MF_UNCHECKED:MF_CHECKED) != 0);

	// Current Line Highlighting
	checkMenuItem(IDM_VIEW_CURLINE_HILITING, _pEditView->isCurrentLineHiLiting());

	// Wrap
	b = _pEditView->isWrap();
	checkMenuItem(IDM_VIEW_WRAP, b);
	_toolBar.setCheck(IDM_VIEW_WRAP, b);
	checkMenuItem(IDM_VIEW_WRAP_SYMBOL, _pEditView->isWrapSymbolVisible());

	//Format conversion
	enableConvertMenuItems(_pEditView->getCurrentBuffer()->getFormat());
	checkUnicodeMenuItems(_pEditView->getCurrentBuffer()->getUnicodeMode());

	//Syncronized scrolling 
}

void Notepad_plus::checkUnicodeMenuItems(UniMode um) const 
{
	int id = -1;
	switch (um)
	{
		case uniUTF8   : id = IDM_FORMAT_UTF_8; break;
		case uni16BE   : id = IDM_FORMAT_UCS_2BE; break;
		case uni16LE   : id = IDM_FORMAT_UCS_2LE; break;
		case uniCookie : id = IDM_FORMAT_AS_UTF_8; break;
		default :
			id = IDM_FORMAT_ANSI;
	}
	::CheckMenuRadioItem(_mainMenuHandle, IDM_FORMAT_ANSI, IDM_FORMAT_AS_UTF_8, id, MF_BYCOMMAND);
}

void Notepad_plus::showAutoComp() {
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showAutoComplete();
}

void Notepad_plus::autoCompFromCurrentFile(bool autoInsert) {
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showWordComplete(autoInsert);
}

void Notepad_plus::showFunctionComp() {
	bool isFromPrimary = _pEditView == &_mainEditView;
	AutoCompletion * autoC = isFromPrimary?&_autoCompleteMain:&_autoCompleteSub;
	autoC->showFunctionComplete();
}

void Notepad_plus::changeMenuLang(generic_string & pluginsTrans, generic_string & windowTrans)
{
	if (!_nativeLangA) return;
	TiXmlNodeA *mainMenu = _nativeLangA->FirstChild("Menu");
	if (!mainMenu) return;
	mainMenu = mainMenu->FirstChild("Main");
	if (!mainMenu) return;
	TiXmlNodeA *entriesRoot = mainMenu->FirstChild("Entries");
	if (!entriesRoot) return;
	const char *idName = NULL;

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		if (element->Attribute("id", &id))
		{
			const char *name = element->Attribute("name");

#ifdef UNICODE
			const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
			::ModifyMenu(_mainMenuHandle, id, MF_BYPOSITION, 0, nameW);
#else
			::ModifyMenu(_mainMenuHandle, id, MF_BYPOSITION, 0, name);
#endif
		}
		else if (idName = element->Attribute("idName"))
		{
			const char *name = element->Attribute("name");
			if (!strcmp(idName, "Plugins"))
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				pluginsTrans = nameW;
#else
				pluginsTrans = name;
#endif
			}
			else if (!strcmp(idName, "Window"))
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				windowTrans = nameW;
#else
				windowTrans = name;
#endif
			}
		}
	}

	TiXmlNodeA *menuCommandsRoot = mainMenu->FirstChild("Commands");
	for (TiXmlNodeA *childNode = menuCommandsRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		element->Attribute("id", &id);
		const char *name = element->Attribute("name");

#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(_mainMenuHandle, id, MF_BYCOMMAND, id, nameW);
#else
		::ModifyMenu(_mainMenuHandle, id, MF_BYCOMMAND, id, name);
#endif
	}

	TiXmlNodeA *subEntriesRoot = mainMenu->FirstChild("SubEntries");

	for (TiXmlNodeA *childNode = subEntriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int x, y;
		element->Attribute("posX", &x);
		element->Attribute("posY", &y);
		const char *name = element->Attribute("name");
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(::GetSubMenu(_mainMenuHandle, x), y, MF_BYPOSITION, 0, nameW);
#else
		::ModifyMenu(::GetSubMenu(_mainMenuHandle, x), y, MF_BYPOSITION, 0, name);
#endif
	}
	::DrawMenuBar(_hSelf);
}

void Notepad_plus::changeLangTabContextMenu()
{
	const int POS_CLOSE = 0;
	const int POS_CLOSEBUT = 1;
	const int POS_SAVE = 2;
	const int POS_SAVEAS = 3;
	const int POS_RENAME = 4;
	const int POS_REMOVE = 5;
	const int POS_PRINT = 6;
	//------7
	const int POS_READONLY = 8;
	const int POS_CLEARREADONLY = 9;
	//------10
	const int POS_CLIPFULLPATH = 11;
	const int POS_CLIPFILENAME = 12;
	const int POS_CLIPCURRENTDIR = 13;
	//------14
	const int POS_GO2VIEW = 15;
	const int POS_CLONE2VIEW = 16;
	const int POS_GO2NEWINST = 17;
	const int POS_OPENINNEWINST = 18;

	const char *pClose = NULL;
	const char *pCloseBut = NULL;
	const char *pSave = NULL;
	const char *pSaveAs = NULL;
	const char *pPrint = NULL;
	const char *pReadOnly = NULL;
	const char *pClearReadOnly = NULL;
	const char *pGoToView = NULL;
	const char *pCloneToView = NULL;
	const char *pGoToNewInst = NULL;
	const char *pOpenInNewInst = NULL;
	const char *pCilpFullPath = NULL;
	const char *pCilpFileName = NULL;
	const char *pCilpCurrentDir = NULL;
	const char *pRename = NULL;
	const char *pRemove = NULL;
	if (_nativeLangA)
	{
		TiXmlNodeA *tabBarMenu = _nativeLangA->FirstChild("Menu");
		if (tabBarMenu) 
		{
			tabBarMenu = tabBarMenu->FirstChild("TabBar");
			if (tabBarMenu)
			{
				for (TiXmlNodeA *childNode = tabBarMenu->FirstChildElement("Item");
					childNode ;
					childNode = childNode->NextSibling("Item") )
				{
					TiXmlElementA *element = childNode->ToElement();
					int ordre;
					element->Attribute("order", &ordre);
					switch (ordre)
					{
						case 0 :
							pClose = element->Attribute("name"); break;
						case 1 :
							pCloseBut = element->Attribute("name"); break;
						case 2 :
							pSave = element->Attribute("name"); break;
						case 3 :
							pSaveAs = element->Attribute("name"); break;
						case 4 :
							pPrint = element->Attribute("name"); break;
						case 5 :
							pGoToView = element->Attribute("name"); break;
						case 6 :
							pCloneToView = element->Attribute("name"); break;
						case 7 :
							pCilpFullPath = element->Attribute("name"); break;
						case 8 :
							pCilpFileName = element->Attribute("name"); break;
						case 9 :
							pCilpCurrentDir = element->Attribute("name"); break;
						case 10 :
							pRename = element->Attribute("name"); break;
						case 11 :
							pRemove = element->Attribute("name"); break;
						case 12 :
							pReadOnly = element->Attribute("name"); break;
						case 13 :
							pClearReadOnly = element->Attribute("name"); break;
						case 14 :
							pGoToNewInst = element->Attribute("name"); break;
						case 15 :
							pOpenInNewInst = element->Attribute("name"); break;
					}
				}
			}	
		}
	}
	HMENU hCM = _tabPopupMenu.getMenuHandle();
	
#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	if (pGoToView && pGoToView[0])
	{
		const wchar_t *goToViewG = wmc->char2wchar(pGoToView, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
		::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION, cmdID, goToViewG);
	}
	if (pCloneToView && pCloneToView[0])
	{
		const wchar_t *cloneToViewG = wmc->char2wchar(pCloneToView, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
		::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION, cmdID, cloneToViewG);
	}
	if (pGoToNewInst && pGoToNewInst[0])
	{
		const wchar_t *goToNewInstG = wmc->char2wchar(pGoToNewInst, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_GO2NEWINST);
		::ModifyMenu(hCM, POS_GO2NEWINST, MF_BYPOSITION, cmdID, goToNewInstG);
	}
	if (pOpenInNewInst && pOpenInNewInst[0])
	{
		const wchar_t *openInNewInstG = wmc->char2wchar(pOpenInNewInst, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_OPENINNEWINST);
		::ModifyMenu(hCM, POS_OPENINNEWINST, MF_BYPOSITION, cmdID, openInNewInstG);
	}
	if (pClose && pClose[0])
	{
		const wchar_t *closeG = wmc->char2wchar(pClose, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSE);
		::ModifyMenu(hCM, POS_CLOSE, MF_BYPOSITION, cmdID, closeG);
	}
	if (pCloseBut && pCloseBut[0])
	{
		const wchar_t *closeButG = wmc->char2wchar(pCloseBut, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSEBUT);
		::ModifyMenu(hCM, POS_CLOSEBUT, MF_BYPOSITION, cmdID, closeButG);
	}
	if (pSave && pSave[0])
	{
		const wchar_t *saveG = wmc->char2wchar(pSave, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_SAVE);
		::ModifyMenu(hCM, POS_SAVE, MF_BYPOSITION, cmdID, saveG);
	}
	if (pSaveAs && pSaveAs[0])
	{
		const wchar_t *saveAsG = wmc->char2wchar(pSaveAs, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_SAVEAS);
		::ModifyMenu(hCM, POS_SAVEAS, MF_BYPOSITION, cmdID, saveAsG);
	}
	if (pPrint && pPrint[0])
	{
		const wchar_t *printG = wmc->char2wchar(pPrint, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_PRINT);
		::ModifyMenu(hCM, POS_PRINT, MF_BYPOSITION, cmdID, printG);
	}
	if (pReadOnly && pReadOnly[0])
	{
		const wchar_t *readOnlyG = wmc->char2wchar(pReadOnly, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_READONLY);
		::ModifyMenu(hCM, POS_READONLY, MF_BYPOSITION, cmdID, readOnlyG);
	}
	if (pClearReadOnly && pClearReadOnly[0])
	{
		const wchar_t *clearReadOnlyG = wmc->char2wchar(pClearReadOnly, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLEARREADONLY);
		::ModifyMenu(hCM, POS_CLEARREADONLY, MF_BYPOSITION, cmdID, clearReadOnlyG);
	}
	if (pCilpFullPath && pCilpFullPath[0])
	{
		const wchar_t *cilpFullPathG = wmc->char2wchar(pCilpFullPath, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFULLPATH);
		::ModifyMenu(hCM, POS_CLIPFULLPATH, MF_BYPOSITION, cmdID, cilpFullPathG);
	}
	if (pCilpFileName && pCilpFileName[0])
	{
		const wchar_t *cilpFileNameG = wmc->char2wchar(pCilpFileName, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFILENAME);
		::ModifyMenu(hCM, POS_CLIPFILENAME, MF_BYPOSITION, cmdID, cilpFileNameG);
	}
	if (pCilpCurrentDir && pCilpCurrentDir[0])
	{
		const wchar_t * cilpCurrentDirG= wmc->char2wchar(pCilpCurrentDir, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPCURRENTDIR);
		::ModifyMenu(hCM, POS_CLIPCURRENTDIR, MF_BYPOSITION, cmdID, cilpCurrentDirG);
	}
	if (pRename && pRename[0])
	{
		const wchar_t *renameG = wmc->char2wchar(pRename, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_RENAME);
		::ModifyMenu(hCM, POS_RENAME, MF_BYPOSITION, cmdID, renameG);
	}
	if (pRemove && pRemove[0])
	{
		const wchar_t *removeG = wmc->char2wchar(pRemove, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_REMOVE);
		::ModifyMenu(hCM, POS_REMOVE, MF_BYPOSITION, cmdID, removeG);
	}
#else
	if (pGoToView && pGoToView[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
		::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION, cmdID, pGoToView);
	}
	if (pCloneToView && pCloneToView[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
		::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION, cmdID, pCloneToView);
	}
	if (pGoToNewInst && pGoToNewInst[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_GO2NEWINST);
		::ModifyMenu(hCM, POS_GO2NEWINST, MF_BYPOSITION, cmdID, pGoToNewInst);
	}
	if (pOpenInNewInst && pOpenInNewInst[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_OPENINNEWINST);
		::ModifyMenu(hCM, POS_OPENINNEWINST, MF_BYPOSITION, cmdID, pOpenInNewInst);
	}
	if (pClose && pClose[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSE);
		::ModifyMenu(hCM, POS_CLOSE, MF_BYPOSITION, cmdID, pClose);
	}
	if (pCloseBut && pCloseBut[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSEBUT);
		::ModifyMenu(hCM, POS_CLOSEBUT, MF_BYPOSITION, cmdID, pCloseBut);
	}
	if (pSave && pSave[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_SAVE);
		::ModifyMenu(hCM, POS_SAVE, MF_BYPOSITION, cmdID, pSave);
	}
	if (pSaveAs && pSaveAs[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_SAVEAS);
		::ModifyMenu(hCM, POS_SAVEAS, MF_BYPOSITION, cmdID, pSaveAs);
	}
	if (pPrint && pPrint[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_PRINT);
		::ModifyMenu(hCM, POS_PRINT, MF_BYPOSITION, cmdID, pPrint);
	}
	if (pClearReadOnly && pClearReadOnly[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLEARREADONLY);
		::ModifyMenu(hCM, POS_CLEARREADONLY, MF_BYPOSITION, cmdID, pClearReadOnly);
	}
	if (pReadOnly && pReadOnly[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_READONLY);
		::ModifyMenu(hCM, POS_READONLY, MF_BYPOSITION, cmdID, pReadOnly);
	}
	if (pCilpFullPath && pCilpFullPath[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFULLPATH);
		::ModifyMenu(hCM, POS_CLIPFULLPATH, MF_BYPOSITION, cmdID, pCilpFullPath);
	}
	if (pCilpFileName && pCilpFileName[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFILENAME);
		::ModifyMenu(hCM, POS_CLIPFILENAME, MF_BYPOSITION, cmdID, pCilpFileName);
	}
	if (pCilpCurrentDir && pCilpCurrentDir[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPCURRENTDIR);
		::ModifyMenu(hCM, POS_CLIPCURRENTDIR, MF_BYPOSITION, cmdID, pCilpCurrentDir);
	}
	if (pRename && pRename[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_RENAME);
		::ModifyMenu(hCM, POS_RENAME, MF_BYPOSITION, cmdID, pRename);
	}
	if (pRemove && pRemove[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_REMOVE);
		::ModifyMenu(hCM, POS_REMOVE, MF_BYPOSITION, cmdID, pRemove);
	}
#endif
}

void Notepad_plus::changeLangTabDrapContextMenu()
{
	const int POS_GO2VIEW = 0;
	const int POS_CLONE2VIEW = 1;
	const char *goToViewA = NULL;
	const char *cloneToViewA = NULL;

	if (_nativeLangA)
	{
		TiXmlNodeA *tabBarMenu = _nativeLangA->FirstChild("Menu");
		if (tabBarMenu)
			tabBarMenu = tabBarMenu->FirstChild("TabBar");
		if (tabBarMenu)
		{
			for (TiXmlNodeA *childNode = tabBarMenu->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElementA *element = childNode->ToElement();
				int ordre;
				element->Attribute("order", &ordre);
				if (ordre == 5)
					goToViewA = element->Attribute("name");
				else if (ordre == 6)
					cloneToViewA = element->Attribute("name");
			}
		}
		HMENU hCM = _tabPopupDropMenu.getMenuHandle();
#ifdef UNICODE
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		if (goToViewA && goToViewA[0])
		{
			const wchar_t *goToViewG = wmc->char2wchar(goToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
			::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION|MF_STRING, cmdID, goToViewG);
		}
		if (cloneToViewA && cloneToViewA[0])
		{
			const wchar_t *cloneToViewG = wmc->char2wchar(cloneToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
			::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION|MF_STRING, cmdID, cloneToViewG);
		}
#else
		if (goToViewA && goToViewA[0])
		{
			int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
			::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION, cmdID, goToViewA);
		}
		if (cloneToViewA && cloneToViewA[0])
		{
			int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
			::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION, cmdID, cloneToViewA);
		}
#endif
	}
}

void Notepad_plus::changeConfigLang()
{
	if (!_nativeLangA) return;

	TiXmlNodeA *styleConfDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!styleConfDlgNode) return;	
	
	styleConfDlgNode = styleConfDlgNode->FirstChild("StyleConfig");
	if (!styleConfDlgNode) return;

	HWND hDlg = _configStyleDlg.getHSelf();

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	// Set Title
	const char *titre = (styleConfDlgNode->ToElement())->Attribute("title");

	if ((titre && titre[0]) && hDlg)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
#else
		::SetWindowText(hDlg, titre);
#endif
	}
	for (TiXmlNodeA *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, name);
#endif
			}
		}
	}
	hDlg = _configStyleDlg.getHSelf();
	styleConfDlgNode = styleConfDlgNode->FirstChild("SubDialog");
	
	for (TiXmlNodeA *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, name);
#endif
			}
		}
	}
}


void Notepad_plus::changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText)
{
	const int iColorStyle = 0;
	const int iUnderline = 8;

	HWND hItem;
	for (int i = iColorStyle ; i < (iUnderline + 1) ; i++)
	{
		if (translatedText[i] && translatedText[i][0])
		{
			hItem = ::GetDlgItem(hDlg, idArray[i]);
			if (hItem)
			{
#ifdef UNICODE
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t *nameW = wmc->char2wchar(translatedText[i], _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, translatedText[i]);
#endif
				
			}
		}
	}
}

void Notepad_plus::changeUserDefineLang()
{
	if (!_nativeLangA) return;

	TiXmlNodeA *userDefineDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!userDefineDlgNode) return;	
	
	userDefineDlgNode = userDefineDlgNode->FirstChild("UserDefine");
	if (!userDefineDlgNode) return;

	UserDefineDialog *userDefineDlg = _pEditView->getUserDefineDlg();

	HWND hDlg = userDefineDlg->getHSelf();
#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	// Set Title
	const char *titre = (userDefineDlgNode->ToElement())->Attribute("title");
	if (titre && titre[0])
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
#else
		::SetWindowText(hDlg, titre);
#endif
	}
	// pour ses propres controls 	
	const int nbControl = 9;
	const char *translatedText[nbControl];
	for (int i = 0 ; i < nbControl ; i++)
		translatedText[i] = NULL;

	for (TiXmlNodeA *childNode = userDefineDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		
		if (sentinel && (name && name[0]))
		{
			if (id > 30)
			{
				HWND hItem = ::GetDlgItem(hDlg, id);
				if (hItem)
				{
#ifdef UNICODE
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					::SetWindowText(hItem, nameW);
#else
					::SetWindowText(hItem, name);
#endif
				}
			}
			else
			{
				switch(id)
				{
					case 0: case 1: case 2: case 3: case 4:
					case 5: case 6: case 7: case 8: 
 						translatedText[id] = name; break;
				}
			}
		}
	}

	const int nbDlg = 4;
	HWND hDlgArrary[nbDlg];
	hDlgArrary[0] = userDefineDlg->getFolderHandle();
	hDlgArrary[1] = userDefineDlg->getKeywordsHandle();
	hDlgArrary[2] = userDefineDlg->getCommentHandle();
	hDlgArrary[3] = userDefineDlg->getSymbolHandle();
	
	const int nbGrpFolder = 3;
	int folderID[nbGrpFolder][nbControl] = {\
		{IDC_DEFAULT_COLORSTYLEGROUP_STATIC, IDC_DEFAULT_FG_STATIC, IDC_DEFAULT_BG_STATIC, IDC_DEFAULT_FONTSTYLEGROUP_STATIC, IDC_DEFAULT_FONTNAME_STATIC, IDC_DEFAULT_FONTSIZE_STATIC, IDC_DEFAULT_BOLD_CHECK, IDC_DEFAULT_ITALIC_CHECK, IDC_DEFAULT_UNDERLINE_CHECK},\
		{IDC_FOLDEROPEN_COLORSTYLEGROUP_STATIC, IDC_FOLDEROPEN_FG_STATIC, IDC_FOLDEROPEN_BG_STATIC, IDC_FOLDEROPEN_FONTSTYLEGROUP_STATIC, IDC_FOLDEROPEN_FONTNAME_STATIC, IDC_FOLDEROPEN_FONTSIZE_STATIC, IDC_FOLDEROPEN_BOLD_CHECK, IDC_FOLDEROPEN_ITALIC_CHECK, IDC_FOLDEROPEN_UNDERLINE_CHECK},\
		{IDC_FOLDERCLOSE_COLORSTYLEGROUP_STATIC, IDC_FOLDERCLOSE_FG_STATIC, IDC_FOLDERCLOSE_BG_STATIC, IDC_FOLDERCLOSE_FONTSTYLEGROUP_STATIC, IDC_FOLDERCLOSE_FONTNAME_STATIC, IDC_FOLDERCLOSE_FONTSIZE_STATIC, IDC_FOLDERCLOSE_BOLD_CHECK, IDC_FOLDERCLOSE_ITALIC_CHECK, IDC_FOLDERCLOSE_UNDERLINE_CHECK}\
	};

	const int nbGrpKeywords = 4;
	int keywordsID[nbGrpKeywords][nbControl] = {\
		 {IDC_KEYWORD1_COLORSTYLEGROUP_STATIC, IDC_KEYWORD1_FG_STATIC, IDC_KEYWORD1_BG_STATIC, IDC_KEYWORD1_FONTSTYLEGROUP_STATIC, IDC_KEYWORD1_FONTNAME_STATIC, IDC_KEYWORD1_FONTSIZE_STATIC, IDC_KEYWORD1_BOLD_CHECK, IDC_KEYWORD1_ITALIC_CHECK, IDC_KEYWORD1_UNDERLINE_CHECK},\
		{IDC_KEYWORD2_COLORSTYLEGROUP_STATIC, IDC_KEYWORD2_FG_STATIC, IDC_KEYWORD2_BG_STATIC, IDC_KEYWORD2_FONTSTYLEGROUP_STATIC, IDC_KEYWORD2_FONTNAME_STATIC, IDC_KEYWORD2_FONTSIZE_STATIC, IDC_KEYWORD2_BOLD_CHECK, IDC_KEYWORD2_ITALIC_CHECK, IDC_KEYWORD2_UNDERLINE_CHECK},\
		{IDC_KEYWORD3_COLORSTYLEGROUP_STATIC, IDC_KEYWORD3_FG_STATIC, IDC_KEYWORD3_BG_STATIC, IDC_KEYWORD3_FONTSTYLEGROUP_STATIC, IDC_KEYWORD3_FONTNAME_STATIC, IDC_KEYWORD3_FONTSIZE_STATIC, IDC_KEYWORD3_BOLD_CHECK, IDC_KEYWORD3_ITALIC_CHECK, IDC_KEYWORD3_UNDERLINE_CHECK},\
		{IDC_KEYWORD4_COLORSTYLEGROUP_STATIC, IDC_KEYWORD4_FG_STATIC, IDC_KEYWORD4_BG_STATIC, IDC_KEYWORD4_FONTSTYLEGROUP_STATIC, IDC_KEYWORD4_FONTNAME_STATIC, IDC_KEYWORD4_FONTSIZE_STATIC, IDC_KEYWORD4_BOLD_CHECK, IDC_KEYWORD4_ITALIC_CHECK, IDC_KEYWORD4_UNDERLINE_CHECK}\
	};

	const int nbGrpComment = 3;
	int commentID[nbGrpComment][nbControl] = {\
		{IDC_COMMENT_COLORSTYLEGROUP_STATIC, IDC_COMMENT_FG_STATIC, IDC_COMMENT_BG_STATIC, IDC_COMMENT_FONTSTYLEGROUP_STATIC, IDC_COMMENT_FONTNAME_STATIC, IDC_COMMENT_FONTSIZE_STATIC, IDC_COMMENT_BOLD_CHECK, IDC_COMMENT_ITALIC_CHECK, IDC_COMMENT_UNDERLINE_CHECK},\
		{IDC_NUMBER_COLORSTYLEGROUP_STATIC, IDC_NUMBER_FG_STATIC, IDC_NUMBER_BG_STATIC, IDC_NUMBER_FONTSTYLEGROUP_STATIC, IDC_NUMBER_FONTNAME_STATIC, IDC_NUMBER_FONTSIZE_STATIC, IDC_NUMBER_BOLD_CHECK, IDC_NUMBER_ITALIC_CHECK, IDC_NUMBER_UNDERLINE_CHECK},\
		{IDC_COMMENTLINE_COLORSTYLEGROUP_STATIC, IDC_COMMENTLINE_FG_STATIC, IDC_COMMENTLINE_BG_STATIC, IDC_COMMENTLINE_FONTSTYLEGROUP_STATIC, IDC_COMMENTLINE_FONTNAME_STATIC, IDC_COMMENTLINE_FONTSIZE_STATIC, IDC_COMMENTLINE_BOLD_CHECK, IDC_COMMENTLINE_ITALIC_CHECK, IDC_COMMENTLINE_UNDERLINE_CHECK}\
	};

	const int nbGrpOperator = 3;
	int operatorID[nbGrpOperator][nbControl] = {\
		{IDC_SYMBOL_COLORSTYLEGROUP_STATIC, IDC_SYMBOL_FG_STATIC, IDC_SYMBOL_BG_STATIC, IDC_SYMBOL_FONTSTYLEGROUP_STATIC, IDC_SYMBOL_FONTNAME_STATIC, IDC_SYMBOL_FONTSIZE_STATIC, IDC_SYMBOL_BOLD_CHECK, IDC_SYMBOL_ITALIC_CHECK, IDC_SYMBOL_UNDERLINE_CHECK},\
		{IDC_SYMBOL_COLORSTYLEGROUP2_STATIC, IDC_SYMBOL_FG2_STATIC, IDC_SYMBOL_BG2_STATIC, IDC_SYMBOL_FONTSTYLEGROUP2_STATIC, IDC_SYMBOL_FONTNAME2_STATIC, IDC_SYMBOL_FONTSIZE2_STATIC, IDC_SYMBOL_BOLD2_CHECK, IDC_SYMBOL_ITALIC2_CHECK, IDC_SYMBOL_UNDERLINE2_CHECK},\
		{IDC_SYMBOL_COLORSTYLEGROUP3_STATIC, IDC_SYMBOL_FG3_STATIC, IDC_SYMBOL_BG3_STATIC, IDC_SYMBOL_FONTSTYLEGROUP3_STATIC, IDC_SYMBOL_FONTNAME3_STATIC, IDC_SYMBOL_FONTSIZE3_STATIC, IDC_SYMBOL_BOLD3_CHECK, IDC_SYMBOL_ITALIC3_CHECK, IDC_SYMBOL_UNDERLINE3_CHECK}
	};
	
	int nbGpArray[nbDlg] = {nbGrpFolder, nbGrpKeywords, nbGrpComment, nbGrpOperator};
	const char nodeNameArray[nbDlg][16] = {"Folder", "Keywords", "Comment", "Operator"};

	for (int i = 0 ; i < nbDlg ; i++)
	{
	
		for (int j = 0 ; j < nbGpArray[i] ; j++)
		{
			switch (i)
			{
				case 0 : changeStyleCtrlsLang(hDlgArrary[i], folderID[j], translatedText); break;
				case 1 : changeStyleCtrlsLang(hDlgArrary[i], keywordsID[j], translatedText); break;
				case 2 : changeStyleCtrlsLang(hDlgArrary[i], commentID[j], translatedText); break;
				case 3 : changeStyleCtrlsLang(hDlgArrary[i], operatorID[j], translatedText); break;
			}
		}
		TiXmlNodeA *node = userDefineDlgNode->FirstChild(nodeNameArray[i]);
		
		if (node) 
		{
			// Set Title
			titre = (node->ToElement())->Attribute("title");
			if (titre &&titre[0])
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
				userDefineDlg->setTabName(i, nameW);
#else
				userDefineDlg->setTabName(i, titre);
#endif
			}
			for (TiXmlNodeA *childNode = node->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElementA *element = childNode->ToElement();
				int id;
				const char *sentinel = element->Attribute("id", &id);
				const char *name = element->Attribute("name");
				if (sentinel && (name && name[0]))
				{
					HWND hItem = ::GetDlgItem(hDlgArrary[i], id);
					if (hItem)
					{
#ifdef UNICODE
						const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
						::SetWindowText(hItem, nameW);
#else
						::SetWindowText(hItem, name);
#endif
					}
				}
			}
		}
	}
}

void Notepad_plus::changeFindReplaceDlgLang()
{
	if (_nativeLangA)
	{
		TiXmlNodeA *dlgNode = _nativeLangA->FirstChild("Dialog");
		if (dlgNode)
		{
			NppParameters *pNppParam = NppParameters::getInstance();
			dlgNode = searchDlgNode(dlgNode, "Find");
			if (dlgNode)
			{
				const char *titre1 = (dlgNode->ToElement())->Attribute("titleFind");
				const char *titre2 = (dlgNode->ToElement())->Attribute("titleReplace");
				const char *titre3 = (dlgNode->ToElement())->Attribute("titleFindInFiles");
				if (titre1 && titre2 && titre3)
				{
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

					basic_string<wchar_t> nameW = wmc->char2wchar(titre1, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._find = nameW;

					nameW = wmc->char2wchar(titre2, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._replace = nameW;

					nameW = wmc->char2wchar(titre3, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._findInFiles = nameW;
#else
					pNppParam->getFindDlgTabTitiles()._find = titre1;
					pNppParam->getFindDlgTabTitiles()._replace = titre2;
					pNppParam->getFindDlgTabTitiles()._findInFiles = titre3;
#endif
				}
			}

			_findReplaceDlg.changeTabName(FIND_DLG, pNppParam->getFindDlgTabTitiles()._find.c_str());
			_findReplaceDlg.changeTabName(REPLACE_DLG, pNppParam->getFindDlgTabTitiles()._replace.c_str());
			_findReplaceDlg.changeTabName(FINDINFILES_DLG, pNppParam->getFindDlgTabTitiles()._findInFiles.c_str());
		}
	}
	changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
}

void Notepad_plus::changePrefereceDlgLang() 
{
	changeDlgLang(_preference.getHSelf(), "Preference");

	char titre[128];

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	changeDlgLang(_preference._barsDlg.getHSelf(), "Global", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("Global"), nameW);
#else
		_preference._ctrlTab.renameTab("Global", titre);
#endif
	}
	changeDlgLang(_preference._marginsDlg.getHSelf(), "Scintillas", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("Scintillas"), nameW);
#else
		_preference._ctrlTab.renameTab("Scintillas", titre);
#endif
	}

	changeDlgLang(_preference._defaultNewDocDlg.getHSelf(), "NewDoc", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("NewDoc"), nameW);
#else
		_preference._ctrlTab.renameTab("NewDoc", titre);
#endif
	}

	changeDlgLang(_preference._fileAssocDlg.getHSelf(), "FileAssoc", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("FileAssoc"), nameW);
#else
		_preference._ctrlTab.renameTab("FileAssoc", titre);
#endif
	}

	changeDlgLang(_preference._langMenuDlg.getHSelf(), "LangMenu", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("LangMenu"), nameW);
#else
		_preference._ctrlTab.renameTab("LangMenu", titre);
#endif
	}

	changeDlgLang(_preference._printSettingsDlg.getHSelf(), "Print1", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("Print1"), nameW);
#else
		_preference._ctrlTab.renameTab("Print1", titre);
#endif
	}
	changeDlgLang(_preference._printSettings2Dlg.getHSelf(), "Print2", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("Print2"), nameW);
#else
		_preference._ctrlTab.renameTab("Print2", titre);
#endif
	}
	changeDlgLang(_preference._settingsDlg.getHSelf(), "MISC", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("MISC"), nameW);
#else
		_preference._ctrlTab.renameTab("MISC", titre);
#endif
	}
	changeDlgLang(_preference._backupDlg.getHSelf(), "Backup", titre);
	if (*titre)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		_preference._ctrlTab.renameTab(TEXT("Backup"), nameW);
#else
		_preference._ctrlTab.renameTab("Backup", titre);
#endif
	}
}

void Notepad_plus::changeShortcutLang()
{
	if (!_nativeLangA) return;

	NppParameters * pNppParam = NppParameters::getInstance();
	vector<CommandShortcut> & mainshortcuts = pNppParam->getUserShortcuts();
	vector<ScintillaKeyMap> & scinshortcuts = pNppParam->getScintillaKeyList();
	int mainSize = (int)mainshortcuts.size();
	int scinSize = (int)scinshortcuts.size();

	TiXmlNodeA *shortcuts = _nativeLangA->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Main");
	if (!shortcuts) return;

	TiXmlNodeA *entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index, id;
		if (element->Attribute("index", &index) && element->Attribute("id", &id))
		{
			if (index > -1 && index < mainSize) { //valid index only
				const char *name = element->Attribute("name");
				CommandShortcut & csc = mainshortcuts[index];
				if (csc.getID() == id) 
				{
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
					csc.setName(nameW);
#else
					csc.setName(name);
#endif
				}
			}
		}
	}

	//Scintilla
	shortcuts = _nativeLangA->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Scintilla");
	if (!shortcuts) return;

	entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < scinSize) { //valid index only
				const char *name = element->Attribute("name");
				ScintillaKeyMap & skm = scinshortcuts[index];
#ifdef UNICODE
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
				skm.setName(nameW);
#else
				skm.setName(name);
#endif
			}
		}
	}

}

void Notepad_plus::changeShortcutmapperLang(ShortcutMapper * sm)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *shortcuts = _nativeLangA->FirstChild("Dialog");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("ShortcutMapper");
	if (!shortcuts) return;

	for (TiXmlNodeA *childNode = shortcuts->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < 5)  //valid index only
			{
				const char *name = element->Attribute("name");

#ifdef UNICODE
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
				sm->translateTab(index, nameW);
#else
				sm->translateTab(index, name);
#endif
			}
		}
	}
}


TiXmlNodeA * searchDlgNode(TiXmlNodeA *node, const char *dlgTagName)
{
	TiXmlNodeA *dlgNode = node->FirstChild(dlgTagName);
	if (dlgNode) return dlgNode;
	for (TiXmlNodeA *childNode = node->FirstChildElement();
		childNode ;
		childNode = childNode->NextSibling() )
	{
		dlgNode = searchDlgNode(childNode, dlgTagName);
		if (dlgNode) return dlgNode;
	}
	return NULL;
}

bool Notepad_plus::changeDlgLang(HWND hDlg, const char *dlgTagName, char *title)
{
	if (title)
		title[0] = '\0';

	if (!_nativeLangA) return false;

	TiXmlNodeA *dlgNode = _nativeLangA->FirstChild("Dialog");
	if (!dlgNode) return false;

	dlgNode = searchDlgNode(dlgNode, dlgTagName);
	if (!dlgNode) return false;

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	// Set Title
	const char *titre = (dlgNode->ToElement())->Attribute("title");
	if ((titre && titre[0]) && hDlg)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
#else
		::SetWindowText(hDlg, titre);
#endif
		if (title)
			strcpy(title, titre);
	}

	// Set the text of child control
	for (TiXmlNodeA *childNode = dlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, name);
#endif
			}
		}
	}
	return true;
}

static generic_string extractSymbol(TCHAR prefix, const TCHAR *str2extract)
{
	bool found = false;
	TCHAR extracted[128] = TEXT("");

	for (int i = 0, j = 0 ; i < lstrlen(str2extract) ; i++)
	{
		if (found)
		{
			if (!str2extract[i] || str2extract[i] == ' ')
			{
				extracted[j] = '\0';
				return generic_string(extracted);
			}
			extracted[j++] = str2extract[i];

		}
		else
		{
			if (!str2extract[i])
				return TEXT("");

			if (str2extract[i] == prefix)
				found = true;
		}
	}
	return  generic_string(extracted);
};

bool Notepad_plus::doBlockComment(comment_mode currCommentMode)
{
	const TCHAR *commentLineSybol;
	generic_string symbol;

	Buffer * buf = _pEditView->getCurrentBuffer();
	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance()->getULCFromName(buf->getUserDefineLangName());
		if (!userLangContainer)
			return false;

		symbol = extractSymbol('0', userLangContainer->_keywordLists[4]);
		commentLineSybol = symbol.c_str();
	}
	else
		commentLineSybol = buf->getCommentLineSymbol();


	if ((!commentLineSybol) || (!commentLineSybol[0]))
		return false;

    generic_string comment(commentLineSybol);
    comment += TEXT(" ");
    generic_string long_comment = comment;
    
	const int linebufferSize = 1000;
    TCHAR linebuf[linebufferSize];
    size_t comment_length = comment.length();
    size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
    size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
    size_t caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
    // checking if caret is located in _beginning_ of selected block
    bool move_caret = caretPosition < selectionEnd;
    int selStartLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionStart);
    int selEndLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionEnd);
    int lines = selEndLine - selStartLine;
    size_t firstSelLineStart = _pEditView->execute(SCI_POSITIONFROMLINE, selStartLine);
    // "caret return" is part of the last selected line
    if ((lines > 0) && (selectionEnd == static_cast<size_t>(_pEditView->execute(SCI_POSITIONFROMLINE, selEndLine))))
		selEndLine--;
    _pEditView->execute(SCI_BEGINUNDOACTION);
    for (int i = selStartLine; i <= selEndLine; i++) 
	{
		int lineStart = _pEditView->execute(SCI_POSITIONFROMLINE, i);
        int lineIndent = lineStart;
        int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, i);
        if ((lineEnd - lineIndent) >= linebufferSize)        // Avoid buffer size problems
                continue;

        lineIndent = _pEditView->execute(SCI_GETLINEINDENTPOSITION, i);
		_pEditView->getGenericText(linebuf, lineIndent, lineEnd);
        
        // empty lines are not commented
        if (lstrlen(linebuf) < 1)
			continue;
   		if (currCommentMode != cm_comment)
		{
			if (memcmp(linebuf, comment.c_str(), comment_length - 1) == 0)
			{
				if (memcmp(linebuf, long_comment.c_str(), comment_length) == 0)
				{
					// removing comment with space after it
					_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + comment_length);
					_pEditView->replaceSelWith("");
					
					if (i == selStartLine) // is this the first selected line?
						selectionStart -= comment_length;
					selectionEnd -= comment_length; // every iteration
					continue;
				}
				else
				{
					// removing comment _without_ space
					_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + comment_length - 1);
					_pEditView->replaceSelWith("");
					if (i == selStartLine) // is this the first selected line?
						selectionStart -= (comment_length - 1);
					selectionEnd -= (comment_length - 1); // every iteration
					continue;
				}
			}
		}
		if ((currCommentMode == cm_toggle) || (currCommentMode == cm_comment))
		{
			if (i == selStartLine) // is this the first selected line?
				selectionStart += comment_length;
			selectionEnd += comment_length; // every iteration
			_pEditView->insertGenericTextFrom(lineIndent, long_comment.c_str());
		}
     }
    // after uncommenting selection may promote itself to the lines
    // before the first initially selected line;
    // another problem - if only comment symbol was selected;
    if (selectionStart < firstSelLineStart)
	{
        if (selectionStart >= selectionEnd - (comment_length - 1))
			selectionEnd = firstSelLineStart;
        selectionStart = firstSelLineStart;
    }
    if (move_caret) 
	{
        // moving caret to the beginning of selected block
        _pEditView->execute(SCI_GOTOPOS, selectionEnd);
        _pEditView->execute(SCI_SETCURRENTPOS, selectionStart);
    }
	else 
	{
        _pEditView->execute(SCI_SETSEL, selectionStart, selectionEnd);
    }
    _pEditView->execute(SCI_ENDUNDOACTION);
    return true;
}

bool Notepad_plus::doStreamComment()
{
	const TCHAR *commentStart;
	const TCHAR *commentEnd;

	generic_string symbolStart;
	generic_string symbolEnd;

	Buffer * buf = _pEditView->getCurrentBuffer();
	if (buf->getLangType() == L_USER)
	{
		UserLangContainer * userLangContainer = NppParameters::getInstance()->getULCFromName(buf->getUserDefineLangName());

		if (!userLangContainer)
			return false;

		symbolStart = extractSymbol('1', userLangContainer->_keywordLists[4]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('2', userLangContainer->_keywordLists[4]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		commentStart = buf->getCommentStart();
		commentEnd = buf->getCommentEnd();
	}

	if ((!commentStart) || (!commentStart[0]))
		return false;
	if ((!commentEnd) || (!commentEnd[0]))
		return false;

	generic_string start_comment(commentStart);
	generic_string end_comment(commentEnd);
	generic_string white_space(TEXT(" "));

	start_comment += white_space;
	white_space += end_comment;
	end_comment = white_space;
	size_t start_comment_length = start_comment.length();
	size_t selectionStart = _pEditView->execute(SCI_GETSELECTIONSTART);
	size_t selectionEnd = _pEditView->execute(SCI_GETSELECTIONEND);
	size_t caretPosition = _pEditView->execute(SCI_GETCURRENTPOS);
	// checking if caret is located in _beginning_ of selected block
	bool move_caret = caretPosition < selectionEnd;
	// if there is no selection?
	if (selectionEnd - selectionStart <= 0)
	{
		int selLine = _pEditView->execute(SCI_LINEFROMPOSITION, selectionStart);
		int lineIndent = _pEditView->execute(SCI_GETLINEINDENTPOSITION, selLine);
		int lineEnd = _pEditView->execute(SCI_GETLINEENDPOSITION, selLine);

		TCHAR linebuf[1000];
		_pEditView->getGenericText(linebuf, lineIndent, lineEnd);
	    
		int caret = _pEditView->execute(SCI_GETCURRENTPOS);
		int line = _pEditView->execute(SCI_LINEFROMPOSITION, caret);
		int lineStart = _pEditView->execute(SCI_POSITIONFROMLINE, line);
		int current = caret - lineStart;
		// checking if we are not inside a word

		int startword = current;
		int endword = current;
		int start_counter = 0;
		int end_counter = 0;
		while (startword > 0)// && wordCharacters.contains(linebuf[startword - 1]))
		{
			start_counter++;
			startword--;
		}
		// checking _beginning_ of the word
		if (startword == current)
				return true; // caret is located _before_ a word
		while (linebuf[endword + 1] != '\0') // && wordCharacters.contains(linebuf[endword + 1]))
		{
			end_counter++;
			endword++;
		}
		selectionStart -= start_counter;
		selectionEnd += (end_counter + 1);
	}
	_pEditView->execute(SCI_BEGINUNDOACTION);
	_pEditView->insertGenericTextFrom(selectionStart, start_comment.c_str());
	selectionEnd += start_comment_length;
	selectionStart += start_comment_length;
	_pEditView->insertGenericTextFrom(selectionEnd, end_comment.c_str());
	if (move_caret)
	{
		// moving caret to the beginning of selected block
		_pEditView->execute(SCI_GOTOPOS, selectionEnd);
		_pEditView->execute(SCI_SETCURRENTPOS, selectionStart);
	}
	else
	{
		_pEditView->execute(SCI_SETSEL, selectionStart, selectionEnd);
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
	return true;
}

bool Notepad_plus::saveScintillaParams(bool whichOne) 
{
	ScintillaViewParams svp;
	ScintillaEditView *pView = (whichOne == SCIV_PRIMARY)?&_mainEditView:&_subEditView;

	svp._lineNumberMarginShow = pView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER); 
	svp._bookMarkMarginShow = pView->hasMarginShowed(ScintillaEditView::_SC_MARGE_SYBOLE);
	svp._indentGuideLineShow = pView->isShownIndentGuide();
	svp._folderStyle = pView->getFolderStyle();
	svp._currentLineHilitingShow = pView->isCurrentLineHiLiting();
	svp._wrapSymbolShow = pView->isWrapSymbolVisible();
	svp._doWrap = pView->isWrap();
	svp._edgeMode = int(pView->execute(SCI_GETEDGEMODE));
	svp._edgeNbColumn = int(pView->execute(SCI_GETEDGECOLUMN));
	svp._zoom = int(pView->execute(SCI_GETZOOM));
	svp._whiteSpaceShow = pView->isInvisibleCharsShown();
	svp._eolShow = pView->isEolVisible();

	return (NppParameters::getInstance())->writeScintillaParams(svp, whichOne);
}

bool Notepad_plus::addCurrentMacro()
{
	vector<MacroShortcut> & theMacros = (NppParameters::getInstance())->getMacroList();
	
	int nbMacro = theMacros.size();

	int cmdID = ID_MACRO + nbMacro;
	MacroShortcut ms(Shortcut(), _macro, cmdID);
	ms.init(_hInst, _hSelf);

	if (ms.doDialog() != -1)
	{
		HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
		int const posBase = 6;	//separator at index 5
		if (nbMacro == 0) 
		{
			::InsertMenu(hMacroMenu, posBase-1, MF_BYPOSITION, (unsigned int)-1, 0);	//no separator yet, add one
		}

		theMacros.push_back(ms);
		::InsertMenu(hMacroMenu, posBase + nbMacro, MF_BYPOSITION, cmdID, ms.toMenuItemString().c_str());
		_accelerator.updateShortcuts();
		return true;
	}
	return false;
}

void Notepad_plus::changeToolBarIcons()
{
	if (!_toolIcons)
		return;
	for (int i = 0 ; i < int(_customIconVect.size()) ; i++)
		_toolBar.changeIcons(_customIconVect[i].listIndex, _customIconVect[i].iconIndex, (_customIconVect[i].iconLocation).c_str());
}

bool Notepad_plus::switchToFile(BufferID id)
{
	int i = 0;
	int iView = currentView();
	if (id == BUFFER_INVALID)
		return false;

	if ((i = _pDocTab->getIndexByBuffer(id)) != -1)
	{
		iView = currentView();
	}
	else if ((i = _pNonDocTab->getIndexByBuffer(id)) != -1)
	{
		iView = otherView();
	}

	if (i != -1)
	{
		switchEditViewTo(iView);
		//_pDocTab->activateAt(i);
		activateBuffer(id, currentView());
		return true;
	}
	return false;
}

ToolBarButtonUnit toolBarIcons[] = {
	{IDM_FILE_NEW,		IDI_NEW_OFF_ICON,		IDI_NEW_ON_ICON,		IDI_NEW_OFF_ICON, IDR_FILENEW},
	{IDM_FILE_OPEN,		IDI_OPEN_OFF_ICON,		IDI_OPEN_ON_ICON,		IDI_NEW_OFF_ICON, IDR_FILEOPEN},
	{IDM_FILE_SAVE,		IDI_SAVE_OFF_ICON,		IDI_SAVE_ON_ICON,		IDI_SAVE_DISABLE_ICON, IDR_FILESAVE},
	{IDM_FILE_SAVEALL,	IDI_SAVEALL_OFF_ICON,	IDI_SAVEALL_ON_ICON,	IDI_SAVEALL_DISABLE_ICON, IDR_SAVEALL},
	{IDM_FILE_CLOSE,	IDI_CLOSE_OFF_ICON,		IDI_CLOSE_ON_ICON,		IDI_CLOSE_OFF_ICON, IDR_CLOSEFILE},
	{IDM_FILE_CLOSEALL,	IDI_CLOSEALL_OFF_ICON,	IDI_CLOSEALL_ON_ICON,	IDI_CLOSEALL_OFF_ICON, IDR_CLOSEALL},
	{IDM_FILE_PRINTNOW,	IDI_PRINT_OFF_ICON,		IDI_PRINT_ON_ICON,		IDI_PRINT_OFF_ICON, IDR_PRINT},
	 
	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_EDIT_CUT,		IDI_CUT_OFF_ICON,		IDI_CUT_ON_ICON,		IDI_CUT_DISABLE_ICON, IDR_CUT},
	{IDM_EDIT_COPY,		IDI_COPY_OFF_ICON,		IDI_COPY_ON_ICON,		IDI_COPY_DISABLE_ICON, IDR_COPY},
	{IDM_EDIT_PASTE,	IDI_PASTE_OFF_ICON,		IDI_PASTE_ON_ICON,		IDI_PASTE_DISABLE_ICON, IDR_PASTE},
	 
	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_EDIT_UNDO,		IDI_UNDO_OFF_ICON,		IDI_UNDO_ON_ICON,		IDI_UNDO_DISABLE_ICON, IDR_UNDO},
	{IDM_EDIT_REDO,		IDI_REDO_OFF_ICON,		IDI_REDO_ON_ICON,		IDI_REDO_DISABLE_ICON, IDR_REDO},	 
	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_SEARCH_FIND,		IDI_FIND_OFF_ICON,		IDI_FIND_ON_ICON,		IDI_FIND_OFF_ICON, IDR_FIND},
	{IDM_SEARCH_REPLACE,  IDI_REPLACE_OFF_ICON,	IDI_REPLACE_ON_ICON,	IDI_REPLACE_OFF_ICON, IDR_REPLACE},
	 
	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	{IDM_VIEW_ZOOMIN,	IDI_ZOOMIN_OFF_ICON,	IDI_ZOOMIN_ON_ICON,		IDI_ZOOMIN_OFF_ICON, IDR_ZOOMIN},
	{IDM_VIEW_ZOOMOUT,	IDI_ZOOMOUT_OFF_ICON,	IDI_ZOOMOUT_ON_ICON,	IDI_ZOOMOUT_OFF_ICON, IDR_ZOOMOUT},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	{IDM_VIEW_SYNSCROLLV,	IDI_SYNCV_OFF_ICON,	IDI_SYNCV_ON_ICON,	IDI_SYNCV_DISABLE_ICON, IDR_SYNCV},
	{IDM_VIEW_SYNSCROLLH,	IDI_SYNCH_OFF_ICON,	IDI_SYNCH_ON_ICON,	IDI_SYNCH_DISABLE_ICON, IDR_SYNCH},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	{IDM_VIEW_WRAP,  IDI_VIEW_WRAP_OFF_ICON,	IDI_VIEW_WRAP_ON_ICON,	IDI_VIEW_WRAP_OFF_ICON, IDR_WRAP},
	{IDM_VIEW_ALL_CHARACTERS,  IDI_VIEW_ALL_CHAR_OFF_ICON,	IDI_VIEW_ALL_CHAR_ON_ICON,	IDI_VIEW_ALL_CHAR_OFF_ICON, IDR_INVISIBLECHAR},
	{IDM_VIEW_INDENT_GUIDE,  IDI_VIEW_INDENT_OFF_ICON,	IDI_VIEW_INDENT_ON_ICON,	IDI_VIEW_INDENT_OFF_ICON, IDR_INDENTGUIDE},
	{IDM_VIEW_USER_DLG,  IDI_VIEW_UD_DLG_OFF_ICON,	IDI_VIEW_UD_DLG_ON_ICON,	IDI_VIEW_UD_DLG_OFF_ICON, IDR_SHOWPANNEL},

	//-------------------------------------------------------------------------------------//
	{0,					IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_MACRO_STARTRECORDINGMACRO,		IDI_STARTRECORD_OFF_ICON,	IDI_STARTRECORD_ON_ICON,	IDI_STARTRECORD_DISABLE_ICON, IDR_STARTRECORD},
	{IDM_MACRO_STOPRECORDINGMACRO,		IDI_STOPRECORD_OFF_ICON,	IDI_STOPRECORD_ON_ICON,		IDI_STOPRECORD_DISABLE_ICON, IDR_STOPRECORD},
	{IDM_MACRO_PLAYBACKRECORDEDMACRO,	IDI_PLAYRECORD_OFF_ICON,	IDI_PLAYRECORD_ON_ICON,		IDI_PLAYRECORD_DISABLE_ICON, IDR_PLAYRECORD},
	{IDM_MACRO_RUNMULTIMACRODLG,			IDI_MMPLAY_OFF_ICON,		IDI_MMPLAY_ON_ICON,			IDI_MMPLAY_DIS_ICON, IDR_M_PLAYRECORD},
	{IDM_MACRO_SAVECURRENTMACRO,			IDI_SAVERECORD_OFF_ICON,	IDI_SAVERECORD_ON_ICON,		IDI_SAVERECORD_DISABLE_ICON, IDR_SAVERECORD}
	
};

void Notepad_plus::getTaskListInfo(TaskListInfo *tli)
{
	size_t currentNbDoc = _pDocTab->nbItem();
	size_t nonCurrentNbDoc = _pNonDocTab->nbItem();
	
	tli->_currentIndex = 0;

	if (!viewVisible(otherView()))
		nonCurrentNbDoc = 0;

	for (size_t i = 0 ; i < currentNbDoc ; i++)
	{
		BufferID bufID = _pDocTab->getBufferByIndex(i);
		Buffer * b = MainFileManager->getBufferByID(bufID);
		int status = b->isReadOnly()?tb_ro:(b->isDirty()?tb_unsaved:tb_saved);
		tli->_tlfsLst.push_back(TaskLstFnStatus(currentView(), i, b->getFullPathName(), status));
	}
	for (size_t i = 0 ; i < nonCurrentNbDoc ; i++)
	{
		BufferID bufID = _pNonDocTab->getBufferByIndex(i);
		Buffer * b = MainFileManager->getBufferByID(bufID);
		int status = b->isReadOnly()?tb_ro:(b->isDirty()?tb_unsaved:tb_saved);
		tli->_tlfsLst.push_back(TaskLstFnStatus(otherView(), i, b->getFullPathName(), status));
	}
}

LRESULT Notepad_plus::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
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
		case WM_CREATE:
		{
			// Menu
			_mainMenuHandle = ::GetMenu(_hSelf);

			//Views
            _pDocTab = &_mainDocTab;
            _pEditView = &_mainEditView;
			_pNonDocTab = &_subDocTab;
			_pNonEditView = &_subEditView;

			_mainEditView.init(_hInst, hwnd);
			_subEditView.init(_hInst, hwnd);

			_fileEditView.init(_hInst, hwnd);
			MainFileManager->init(this, &_fileEditView);	//get it up and running asap.

			pNppParam->setFontList(hwnd);
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();

			_mainWindowStatus = WindowMainActive;
			_activeView = MAIN_VIEW;

            const ScintillaViewParams & svp1 = pNppParam->getSVP(SCIV_PRIMARY);
			const ScintillaViewParams & svp2 = pNppParam->getSVP(SCIV_SECOND);

			int tabBarStatus = nppGUI._tabStatus;
			_toReduceTabBar = ((tabBarStatus & TAB_REDUCE) != 0);
			_docTabIconList.create(_toReduceTabBar?13:20, _hInst, docTabIconIDs, sizeof(docTabIconIDs)/sizeof(int));

			_mainDocTab.init(_hInst, hwnd, &_mainEditView, &_docTabIconList);
			_subDocTab.init(_hInst, hwnd, &_subEditView, &_docTabIconList);

			_mainEditView.display();

			_mainEditView.execute(SCI_MARKERSETALPHA, MARK_BOOKMARK, 70);
			_mainEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_BOOKMARK, (LPARAM)bookmark_xpm);
			_mainEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESBEGIN, (LPARAM)acTop_xpm);
			_mainEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESEND, (LPARAM)acBottom_xpm);
			_subEditView.execute(SCI_MARKERSETALPHA, MARK_BOOKMARK, 70);
			_subEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_BOOKMARK, (LPARAM)bookmark_xpm);
			_subEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESBEGIN, (LPARAM)acTop_xpm);
			_subEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESEND, (LPARAM)acBottom_xpm);

			_invisibleEditView.init(_hInst, hwnd);
			_invisibleEditView.execute(SCI_SETUNDOCOLLECTION);
			_invisibleEditView.execute(SCI_EMPTYUNDOBUFFER);

			// Configuration of 2 scintilla views
            _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp1._lineNumberMarginShow);
			_subEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp2._lineNumberMarginShow);
            _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp1._bookMarkMarginShow);
			_subEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp2._bookMarkMarginShow);

            _mainEditView.showIndentGuideLine(svp1._indentGuideLineShow);
            _subEditView.showIndentGuideLine(svp2._indentGuideLineShow);
			
			::SendMessage(hwnd, NPPM_INTERNAL_SETCARETWIDTH, 0, 0);
			::SendMessage(hwnd, NPPM_INTERNAL_SETCARETBLINKRATE, 0, 0);

			_configStyleDlg.init(_hInst, _hSelf);
			_preference.init(_hInst, _hSelf);
			
            //Marker Margin config
            _mainEditView.setMakerStyle(svp1._folderStyle);
            _subEditView.setMakerStyle(svp2._folderStyle);

			_mainEditView.execute(SCI_SETCARETLINEVISIBLE, svp1._currentLineHilitingShow);
			_subEditView.execute(SCI_SETCARETLINEVISIBLE, svp2._currentLineHilitingShow);
			
			_mainEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
			_subEditView.execute(SCI_SETCARETLINEVISIBLEALWAYS, true);

			_mainEditView.wrap(svp1._doWrap);
			_subEditView.wrap(svp2._doWrap);

			_mainEditView.execute(SCI_SETEDGECOLUMN, svp1._edgeNbColumn);
			_mainEditView.execute(SCI_SETEDGEMODE, svp1._edgeMode);
			_subEditView.execute(SCI_SETEDGECOLUMN, svp2._edgeNbColumn);
			_subEditView.execute(SCI_SETEDGEMODE, svp2._edgeMode);

			_mainEditView.showEOL(svp1._eolShow);
			_subEditView.showEOL(svp2._eolShow);

			_mainEditView.showWSAndTab(svp1._whiteSpaceShow);
			_subEditView.showWSAndTab(svp2._whiteSpaceShow);

			_mainEditView.showWrapSymbol(svp1._wrapSymbolShow);
			_subEditView.showWrapSymbol(svp2._wrapSymbolShow);

			_mainEditView.performGlobalStyles();
			_subEditView.performGlobalStyles();

			_zoomOriginalValue = _pEditView->execute(SCI_GETZOOM);
			_mainEditView.execute(SCI_SETZOOM, svp1._zoom);
			_subEditView.execute(SCI_SETZOOM, svp2._zoom);

			TabBarPlus::doDragNDrop(true);

			if (_toReduceTabBar)
			{
				HFONT hf = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

				if (hf)
				{
					::SendMessage(_mainDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
					::SendMessage(_subDocTab.getHSelf(), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));
				}
				TabCtrl_SetItemSize(_mainDocTab.getHSelf(), 45, 20);
				TabCtrl_SetItemSize(_subDocTab.getHSelf(), 45, 20);
			}
			_mainDocTab.display();

			
			TabBarPlus::doDragNDrop((tabBarStatus & TAB_DRAGNDROP) != 0);
			TabBarPlus::setDrawTopBar((tabBarStatus & TAB_DRAWTOPBAR) != 0);
			TabBarPlus::setDrawInactiveTab((tabBarStatus & TAB_DRAWINACTIVETAB) != 0);
			TabBarPlus::setDrawTabCloseButton((tabBarStatus & TAB_CLOSEBUTTON) != 0);
			TabBarPlus::setDbClk2Close((tabBarStatus & TAB_DBCLK2CLOSE) != 0);
			TabBarPlus::setVertical((tabBarStatus & TAB_VERTICAL) != 0);
			drawTabbarColoursFromStylerArray();

            //--Splitter Section--//
			bool isVertical = (nppGUI._splitterPos == POS_VERTICAL);

            _subSplitter.init(_hInst, _hSelf);
            _subSplitter.create(&_mainDocTab, &_subDocTab, 8, DYNAMIC, 50, isVertical);

            //--Status Bar Section--//
			bool willBeShown = nppGUI._statusBarShow;
            _statusBar.init(_hInst, hwnd, 6);
			_statusBar.setPartWidth(STATUSBAR_DOC_SIZE, 180);
			_statusBar.setPartWidth(STATUSBAR_CUR_POS, 250);
			_statusBar.setPartWidth(STATUSBAR_EOF_FORMAT, 80);
			_statusBar.setPartWidth(STATUSBAR_UNICODE_TYPE, 100);
			_statusBar.setPartWidth(STATUSBAR_TYPING_MODE, 30);
            _statusBar.display(willBeShown);

            _pMainWindow = &_mainDocTab;

			_dockingManager.init(_hInst, hwnd, &_pMainWindow);

			if (nppGUI._isMinimizedToTray)
				_pTrayIco = new trayIconControler(_hSelf, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

			checkSyncState();

			// Plugin Manager
			NppData nppData;
			nppData._nppHandle = _hSelf;
			nppData._scintillaMainHandle = _mainEditView.getHSelf();
			nppData._scintillaSecondHandle = _subEditView.getHSelf();

			_scintillaCtrls4Plugins.init(_hInst, hwnd);
			_pluginsManager.init(nppData);
			_pluginsManager.loadPlugins();
			const TCHAR *appDataNpp = pNppParam->getAppDataNppDir();
			if (appDataNpp[0])
				_pluginsManager.loadPlugins(appDataNpp);

			// ------------ //
			// Menu Section //
			// ------------ //

			// Macro Menu
			std::vector<MacroShortcut> & macros  = pNppParam->getMacroList();
			HMENU hMacroMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_MACRO);
			size_t const posBase = 6;
			size_t nbMacro = macros.size();
			if (nbMacro >= 1)
				::InsertMenu(hMacroMenu, posBase - 1, MF_BYPOSITION, (unsigned int)-1, 0);
			for (size_t i = 0 ; i < nbMacro ; i++)
			{
				::InsertMenu(hMacroMenu, posBase + i, MF_BYPOSITION, ID_MACRO + i, macros[i].toMenuItemString().c_str());
			}
			// Run Menu
			std::vector<UserCommand> & userCommands = pNppParam->getUserCommandList();
			HMENU hRunMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_RUN);
			int const runPosBase = 2;
			size_t nbUserCommand = userCommands.size();
			if (nbUserCommand >= 1)
				::InsertMenu(hRunMenu, runPosBase - 1, MF_BYPOSITION, (unsigned int)-1, 0);
			for (size_t i = 0 ; i < nbUserCommand ; i++)
			{
				::InsertMenu(hRunMenu, runPosBase + i, MF_BYPOSITION, ID_USER_CMD + i, userCommands[i].toMenuItemString().c_str());
			}

			// Updater menu item
			if (!nppGUI._doesExistUpdater)
			{
				//::MessageBox(NULL, TEXT("pas de updater"), TEXT(""), MB_OK);
				::DeleteMenu(_mainMenuHandle, IDM_UPDATE_NPP, MF_BYCOMMAND);
				::DrawMenuBar(hwnd);
			}
			//Languages Menu
			HMENU hLangMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_LANGUAGE);

			// Add external languages to menu
			for (int i = 0 ; i < pNppParam->getNbExternalLang() ; i++)
			{
				ExternalLangContainer & externalLangContainer = pNppParam->getELCFromIndex(i);

				int numLangs = ::GetMenuItemCount(hLangMenu);
				const int bufferSize = 100;
				TCHAR buffer[bufferSize];

				int x;
				for(x = 0; (x == 0 || lstrcmp(externalLangContainer._name, buffer) > 0) && x < numLangs; x++)
				{
					::GetMenuString(hLangMenu, x, buffer, bufferSize, MF_BYPOSITION);
				}

				::InsertMenu(hLangMenu, x-1, MF_BYPOSITION, IDM_LANG_EXTERNAL + i, externalLangContainer._name);
			}

			if (nppGUI._excludedLangList.size() > 0)
			{
				for (size_t i = 0 ; i < nppGUI._excludedLangList.size() ; i++)
				{
					int cmdID = pNppParam->langTypeToCommandID(nppGUI._excludedLangList[i]._langType);
					const int itemSize = 256;
					TCHAR itemName[itemSize];
					::GetMenuString(hLangMenu, cmdID, itemName, itemSize, MF_BYCOMMAND);
					nppGUI._excludedLangList[i]._cmdID = cmdID;
					nppGUI._excludedLangList[i]._langName = itemName;
					::DeleteMenu(hLangMenu, cmdID, MF_BYCOMMAND);
					DrawMenuBar(_hSelf);
				}
			}

			// Add User Define Languages Entry
			int udlpos = ::GetMenuItemCount(hLangMenu) - 1;

			for (int i = 0 ; i < pNppParam->getNbUserLang() ; i++)
			{
				UserLangContainer & userLangContainer = pNppParam->getULCFromIndex(i);
				::InsertMenu(hLangMenu, udlpos + i, MF_BYPOSITION, IDM_LANG_USER + i + 1, userLangContainer.getName());
			}

			//Add recent files
			HMENU hFileMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FILE);
			int nbLRFile = pNppParam->getNbLRFile();
			int pos = IDM_FILEMENU_LASTONE - IDM_FILE + 2;

			_lastRecentFileList.initMenu(hFileMenu, IDM_FILEMENU_LASTONE + 1, pos);
			for (int i = 0 ; i < nbLRFile ; i++)
			{
				generic_string * stdStr = pNppParam->getLRFile(i);
				if (nppGUI._checkHistoryFiles)
				{
					if (PathFileExists(stdStr->c_str()))
					{
						_lastRecentFileList.add(stdStr->c_str());
					}
				}
				else
				{
					_lastRecentFileList.add(stdStr->c_str());
				}
			}

			//Plugin menu
			_pluginsManager.setMenu(_mainMenuHandle, NULL);
			
			//Main menu is loaded, now load context menu items
			pNppParam->getContextMenuFromXmlTree(_mainMenuHandle);

			if (pNppParam->hasCustomContextMenu())
			{
				_mainEditView.execute(SCI_USEPOPUP, FALSE);
				_subEditView.execute(SCI_USEPOPUP, FALSE);
			}

			generic_string pluginsTrans, windowTrans;
			changeMenuLang(pluginsTrans, windowTrans);
			
			if (_pluginsManager.hasPlugins() && pluginsTrans != TEXT(""))
			{
				::ModifyMenu(_mainMenuHandle, MENUINDEX_PLUGINS, MF_BYPOSITION, 0, pluginsTrans.c_str());
			}
			//Windows menu
			_windowsMenu.init(_hInst, _mainMenuHandle, windowTrans.c_str());

			// Update context menu strings
			vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
			size_t len = tmp.size();
			TCHAR menuName[64];
			for (size_t i = 0 ; i < len ; i++)
			{
				if (tmp[i]._itemName == TEXT(""))
				{
					::GetMenuString(_mainMenuHandle, tmp[i]._cmdID, menuName, 64, MF_BYCOMMAND);
					tmp[i]._itemName = purgeMenuItemString(menuName);
				}
			}

			//Input all the menu item names into shortcut list
			//This will automatically do all translations, since menu translation has been done already
			vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
			len = shortcuts.size();

			for(size_t i = 0; i < len; i++) 
			{
				CommandShortcut & csc = shortcuts[i];
				if (!csc.getName()[0]) 
				{	//no predefined name, get name from menu and use that
					::GetMenuString(_mainMenuHandle, csc.getID(), menuName, 64, MF_BYCOMMAND);
					csc.setName(purgeMenuItemString(menuName, true).c_str());
				}
			}
			//Translate non-menu shortcuts
			changeShortcutLang();

			//Update plugin shortcuts, all plugin commands should be available now
			pNppParam->reloadPluginCmds();

			// Shortcut Accelerator : should be the last one since it will capture all the shortcuts
			_accelerator.init(_mainMenuHandle, _hSelf);
			pNppParam->setAccelerator(&_accelerator);
			
			// Scintilla key accelerator
			vector<HWND> scints;
			scints.push_back(_mainEditView.getHSelf());
			scints.push_back(_subEditView.getHSelf());
			_scintaccelerator.init(&scints, _mainMenuHandle, _hSelf);

			pNppParam->setScintillaAccelerator(&_scintaccelerator);
			_scintaccelerator.updateKeys();

			::DrawMenuBar(_hSelf);


            //-- Tool Bar Section --//
			toolBarStatusType tbStatus = nppGUI._toolBarStatus;
			willBeShown = nppGUI._toolbarShow;
			
			// To notify plugins that toolbar icons can be registered
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_TBMODIFICATION;
			scnN.nmhdr.hwndFrom = _hSelf;
			scnN.nmhdr.idFrom = 0;
			_pluginsManager.notify(&scnN);

			_toolBar.init(_hInst, hwnd, tbStatus, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit));

			changeToolBarIcons();

			_rebarTop.init(_hInst, hwnd);
			_rebarBottom.init(_hInst, hwnd);
			_toolBar.addToRebar(&_rebarTop);
			_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, willBeShown);

			//--Init dialogs--//
            _findReplaceDlg.init(_hInst, hwnd, &_pEditView);
			_incrementFindDlg.init(_hInst, hwnd, &_findReplaceDlg, _isRTL);
			_incrementFindDlg.addToRebar(&_rebarBottom);
            _goToLineDlg.init(_hInst, hwnd, &_pEditView);
			_colEditorDlg.init(_hInst, hwnd, &_pEditView);
            _aboutDlg.init(_hInst, hwnd);
			_runDlg.init(_hInst, hwnd);
			_runMacroDlg.init(_hInst, hwnd);

            //--User Define Dialog Section--//
			int uddStatus = nppGUI._userDefineDlgStatus;
		    UserDefineDialog *udd = _pEditView->getUserDefineDlg();

			bool uddShow = false;
			switch (uddStatus)
            {
                case UDD_SHOW :                 // show & undocked
					udd->doDialog(true, _isRTL);
					changeUserDefineLang();
					uddShow = true;
                    break;
                case UDD_DOCKED : {              // hide & docked
					_isUDDocked = true;
                    break;}
                case (UDD_SHOW | UDD_DOCKED) :    // show & docked
		            udd->doDialog(true, _isRTL);
					changeUserDefineLang();
		            ::SendMessage(udd->getHSelf(), WM_COMMAND, IDC_DOCK_BUTTON, 0);
					uddShow = true;
                    break;

				default :                        // hide & undocked
					break;
            }
            		// UserDefine Dialog
			
			checkMenuItem(IDM_VIEW_USER_DLG, uddShow);
			_toolBar.setCheck(IDM_VIEW_USER_DLG, uddShow);

			//launch the plugin dlg memorized at the last session
			DockingManagerData &dmd = nppGUI._dockingData;

			_dockingManager.setDockedContSize(CONT_LEFT  , nppGUI._dockingData._leftWidth);
			_dockingManager.setDockedContSize(CONT_RIGHT , nppGUI._dockingData._rightWidth);
			_dockingManager.setDockedContSize(CONT_TOP	 , nppGUI._dockingData._topHeight);
			_dockingManager.setDockedContSize(CONT_BOTTOM, nppGUI._dockingData._bottomHight);

			for (size_t i = 0 ; i < dmd._pluginDockInfo.size() ; i++)
			{
				PlugingDlgDockingInfo & pdi = dmd._pluginDockInfo[i];

				if (pdi._isVisible)
					_pluginsManager.runPluginCommand(pdi._name, pdi._internalID);
			}

			for (size_t i = 0 ; i < dmd._containerTabInfo.size() ; i++)
			{
				ContainerTabInfo & cti = dmd._containerTabInfo[i];
				_dockingManager.setActiveTab(cti._cont, cti._activeTab);
			}
			//Load initial docs into doctab
			loadBufferIntoView(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
			loadBufferIntoView(_subEditView.getCurrentBufferID(), SUB_VIEW);
			activateBuffer(_mainEditView.getCurrentBufferID(), MAIN_VIEW);
			activateBuffer(_subEditView.getCurrentBufferID(), SUB_VIEW);
			MainFileManager->increaseDocNr();	//so next doc starts at 2

			::SetFocus(_mainEditView.getHSelf());
			result = TRUE;
		}
		break;

		case WM_DRAWITEM :
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			if (dis->CtlType == ODT_TAB)
			{
				return ::SendMessage(dis->hwndItem, WM_DRAWITEM, wParam, lParam);
			}
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
            TCHAR name[256];
			lstrcpy(name, (TCHAR *)lParam);
			//loop through buffers and reset the language (L_USER, TEXT("")) if (L_USER, name)
			Buffer * buf;
			for(int i = 0; i < MainFileManager->getNrBuffers(); i++) {
				buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && !lstrcmp(buf->getUserDefineLangName(), name))
					buf->setLangType(L_USER, TEXT(""));
			}
			return TRUE;
		}

        case WM_RENAME_USERLANG:
		{
            TCHAR oldName[256];
			TCHAR newName[256];
			lstrcpy(oldName, (TCHAR *)lParam);
			lstrcpy(newName, (TCHAR *)wParam);
			//loop through buffers and reset the language (L_USER, newName) if (L_USER, oldName)
			Buffer * buf;
			for(int i = 0; i < MainFileManager->getNrBuffers(); i++) {
				buf = MainFileManager->getBufferByIndex(i);
				if (buf->getLangType() == L_USER && !lstrcmp(buf->getUserDefineLangName(), oldName))
					buf->setLangType(L_USER, newName);
			}
			return TRUE;
		}

		case WM_CLOSE_USERDEFINE_DLG :
		{
			checkMenuItem(IDM_VIEW_USER_DLG, false);
			_toolBar.setCheck(IDM_VIEW_USER_DLG, false);
			return TRUE;
		}

		case WM_REPLACEALL_INOPENEDDOC :
		{
			replaceAllFiles();
			return TRUE;
		}

		case WM_FINDALL_INOPENEDDOC :
		{
			findInOpenedFiles();
			return TRUE;
		}

		case WM_FINDINFILES :
		{
			findInFiles();
			return TRUE;
		}

		case NPPM_LAUNCHFINDINFILESDLG :
		{
			const int strSize = 64;
			TCHAR str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _isRTL);

			_pEditView->getGenericSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str, _pEditView->getCurrentBuffer()->getUnicodeMode() != uni8Bit);
			if (isFirstTime)
				changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
			_findReplaceDlg.launchFindInFilesDlg();
			
			const TCHAR *dir = NULL;
			generic_string fltr;

			if (wParam)
				dir = (const TCHAR *)wParam;
			else
			{
				dir = NppParameters::getInstance()->getWorkingDir();
			}

			if (lParam)
			{
				fltr = (const TCHAR *)lParam;
			}
			else
			{
				LangType lt = _pEditView->getCurrentBuffer()->getLangType();

				const TCHAR *ext = NppParameters::getInstance()->getLangExtFromLangType(lt);
				if (ext && ext[0])
				{
					generic_string filtres = TEXT("");
					vector<generic_string> vStr;
					cutString(ext, vStr);
					for (size_t i = 0 ; i < vStr.size() ; i++)
					{
							filtres += TEXT("*.");
							filtres += vStr[i] + TEXT(" ");
					}
					fltr = filtres;
				}
				else
					fltr = TEXT("*.*");
			}
			_findReplaceDlg.setFindInFilesDirFilter(dir, fltr.c_str());
			return TRUE;
		}

		case WM_DOOPEN:
		{
			BufferID id = doOpen((const TCHAR *)lParam);
			if (id != BUFFER_INVALID) {
				return switchToFile(id);
			}
		}
		break;

		case NPPM_SETFILENAME:
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
		break;

		case NPPM_GETBUFFERLANGTYPE:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getLangType();
		}
		break;

		case NPPM_SETBUFFERLANGTYPE:
		{
			if (!wParam)
				return FALSE;
			if (lParam < L_TXT || lParam >= L_EXTERNAL || lParam == L_USER)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setLangType((LangType)lParam);
			return TRUE;
		}
		break;

		case NPPM_GETBUFFERENCODING:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getUnicodeMode();
		}
		break;

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
		break;

		case NPPM_GETBUFFERFORMAT:
		{
			if (!wParam)
				return -1;
			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			return b->getFormat();
		}
		break;

		case NPPM_SETBUFFERFORMAT:
		{
			if (!wParam)
				return FALSE;
			if (lParam < WIN_FORMAT || lParam >= UNIX_FORMAT)
				return FALSE;

			BufferID id = (BufferID)wParam;
			Buffer * b = MainFileManager->getBufferByID(id);
			b->setFormat((formatType)lParam);
			return TRUE;
		}
		break;

		case NPPM_GETBUFFERIDFROMPOS:
		{
			DocTabView * pView = NULL;
			if (lParam == MAIN_VIEW) {
				pView = &_mainDocTab;
			} else if (lParam == SUB_VIEW) {
				pView = &_subDocTab;
			} else {
				return (LRESULT)BUFFER_INVALID;
			}
			if ((int)wParam < pView->nbItem()) {
				return (LRESULT)(pView->getBufferByIndex((int)wParam));
			}
			return (LRESULT)BUFFER_INVALID;
		}
		break;

		case NPPM_GETCURRENTBUFFERID:
		{
			return (LRESULT)(_pEditView->getCurrentBufferID());
		}
		break;

		case NPPM_RELOADBUFFERID:
		{
			if (!wParam)
				return FALSE;
			return doReload((BufferID)wParam, lParam != 0);
		}
		break;

		case NPPM_RELOADFILE:
		{
			BufferID id = MainFileManager->getBufferFromName((const TCHAR *)lParam);
			if (id != BUFFER_INVALID)
				doReload(id, wParam != 0);
		}
		break;

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
		break;

		case NPPM_SAVEALLFILES:
		{
			return fileSaveAll();
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			getClientRect(rc);
			if (lParam == 0) {
				lParam = MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top);
			}

			::MoveWindow(_rebarTop.getHSelf(), 0, 0, rc.right, _rebarTop.getHeight(), TRUE);
			_statusBar.adjustParts(rc.right);
			::SendMessage(_statusBar.getHSelf(), WM_SIZE, wParam, lParam);

			int rebarBottomHeight = _rebarBottom.getHeight();
			int statusBarHeight = _statusBar.getHeight();
			::MoveWindow(_rebarBottom.getHSelf(), 0, rc.bottom - rebarBottomHeight - statusBarHeight, rc.right, rebarBottomHeight, TRUE);
			
			getMainClientRect(rc);
			_dockingManager.reSizeTo(rc);

			result = TRUE;
		}
		break;

		case WM_MOVE:
		{
			result = TRUE;
		}
		break;

		case WM_MOVING:
		{
			result = FALSE;
		}
		break;

		case WM_SIZING:
		{
			result = FALSE;
		}
		break;
		
		case WM_COPYDATA :
        {
            COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT *)lParam;

			switch (pCopyData->dwData)
			{
				case COPYDATA_PARAMS :
				{
					pNppParam->setCmdlineParam(*((CmdLineParams *)pCopyData->lpData));
					break;
				}

				case COPYDATA_FILENAMESA :
				{
					char *fileNamesA = (char *)pCopyData->lpData;
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t *fileNamesW = wmc->char2wchar(fileNamesA, CP_ACP);
					loadCommandlineParams(fileNamesW, &cmdLineParams);
#else
					loadCommandlineParams(fileNamesA, &cmdLineParams);
#endif
					break;
				}

				case COPYDATA_FILENAMESW :
				{
					wchar_t *fileNamesW = (wchar_t *)pCopyData->lpData;
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
					
#ifdef UNICODE
					loadCommandlineParams(fileNamesW, &cmdLineParams);
#else
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const char *fileNamesA = wmc->wchar2char(fileNamesW, CP_ACP);
					loadCommandlineParams(fileNamesA, &cmdLineParams);
#endif
					break;
				}
			}

            return TRUE;
        }

		case WM_COMMAND:
            if (HIWORD(wParam) == SCEN_SETFOCUS)
            {
                switchEditViewTo((lParam == (LPARAM)_mainEditView.getHSelf())?MAIN_VIEW:SUB_VIEW);
            }
            else
			{
				if ((lParam == 1) || (lParam == 2))
				{
					specialCmd(LOWORD(wParam), lParam);
				}
				else
					command(LOWORD(wParam));
			}
			return TRUE;

		case NPPM_INTERNAL_RELOADNATIVELANG:
		{
			reloadLang();
		}
		return TRUE;

		case NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED:
		{
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_SHORTCUTREMAPPED;
			scnN.nmhdr.hwndFrom = (void *)lParam; // ShortcutKey structure
			scnN.nmhdr.idFrom = (uptr_t)wParam; // cmdID
			_pluginsManager.notify(&scnN);
		}
		return TRUE;

		case NPPM_GETSHORTCUTBYCMDID:
		{
			int cmdID = wParam; // cmdID
			ShortcutKey *sk = (ShortcutKey *)lParam; // ShortcutKey structure

			return _pluginsManager.getShortcutByCmdID(cmdID, sk);
		}

		case NPPM_MENUCOMMAND :
			command(lParam);
			return TRUE;

		case NPPM_GETFULLCURRENTPATH :
		case NPPM_GETCURRENTDIRECTORY :
		case NPPM_GETFILENAME :
		case NPPM_GETNAMEPART :
		case NPPM_GETEXTPART :
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
					::MessageBox(_hSelf, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM error"), MB_OK);
					return FALSE;
				}
			}

			lstrcpy((TCHAR *)lParam, fileStr);
			return TRUE;
		}

		case NPPM_GETCURRENTWORD :
		{
			const int strSize = MAX_PATH;
			TCHAR str[strSize];

			_pEditView->getGenericSelectedText((TCHAR *)str, strSize);
			// For the compability reason, if wParam is 0, then we assume the size of generic_string buffer (lParam) is large enough.
			// otherwise we check if the generic_string buffer size is enough for the generic_string to copy.
			if (wParam != 0)
			{
				if (lstrlen(str) >= int(wParam))	//buffer too small
				{
					::MessageBox(_hSelf, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM_GETCURRENTWORD error"), MB_OK);
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

		case NPPM_GETNPPDIRECTORY :
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
					::MessageBox(_hSelf, TEXT("Allocated buffer size is not enough to copy the string."), TEXT("NPPM_GETNPPDIRECTORY error"), MB_OK);
					return FALSE;
				}
			}

			lstrcpy((TCHAR *)lParam, str);
			return TRUE;
		}

		case NPPM_GETCURRENTLINE :
		{
			return _pEditView->getCurrentLineNumber();
		}

		case NPPM_GETCURRENTCOLUMN :
		{
			return _pEditView->getCurrentColumnNumber();
		}

		case NPPM_GETCURRENTSCINTILLA :
		{
			*((int *)lParam) = (_pEditView == &_mainEditView)?0:1;
			return TRUE;
		}

		case NPPM_GETCURRENTLANGTYPE :
		{
			*((LangType *)lParam) = _pEditView->getCurrentBuffer()->getLangType();
			return TRUE;
		}

		case NPPM_SETCURRENTLANGTYPE :
		{
			_pEditView->getCurrentBuffer()->setLangType((LangType)lParam);
			return TRUE;
		}

		case NPPM_GETNBOPENFILES :
		{
			int nbDocPrimary = _mainDocTab.nbItem();
			int nbDocSecond = _subDocTab.nbItem();
			if (lParam == ALL_OPEN_FILES)
				return nbDocPrimary + nbDocSecond;
			else if (lParam == PRIMARY_VIEW)
				return  nbDocPrimary;
			else if (lParam == SECOND_VIEW)
				return  nbDocSecond;
		}

		case NPPM_GETOPENFILENAMESPRIMARY :
		case NPPM_GETOPENFILENAMESSECOND :
		case NPPM_GETOPENFILENAMES :
		{
			if (!wParam) return 0;

			TCHAR **fileNames = (TCHAR **)wParam;
			int nbFileNames = lParam;

			int j = 0;
			if (Message != NPPM_GETOPENFILENAMESSECOND) {
				for (int i = 0 ; i < _mainDocTab.nbItem() && j < nbFileNames ; i++)
				{
					BufferID id = _mainDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}
			if (Message != NPPM_GETOPENFILENAMESPRIMARY) {
				for (int i = 0 ; i < _subDocTab.nbItem() && j < nbFileNames ; i++)
				{
					BufferID id = _subDocTab.getBufferByIndex(i);
					Buffer * buf = MainFileManager->getBufferByID(id);
					lstrcpy(fileNames[j++], buf->getFullPathName());
				}
			}
			return j;
		}

		case WM_GETTASKLISTINFO :
		{
			if (!wParam) return 0;
			TaskListInfo * tli = (TaskListInfo *)wParam;
			getTaskListInfo(tli);

			if (NppParameters::getInstance()->getNppGUI()._styleMRU)
			{
				tli->_currentIndex = 0;
				std::sort(tli->_tlfsLst.begin(),tli->_tlfsLst.end(),SortTaskListPred(_mainDocTab,_subDocTab));
			}
			else
			{
				for(int idx = 0; idx < (int)tli->_tlfsLst.size(); ++idx)
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

		case WM_MOUSEWHEEL :
		{
			if (LOWORD(wParam) & MK_RBUTTON)
			{
				// redirect to the IDC_PREV_DOC or IDC_NEXT_DOC so that we have the unified process

				pNppParam->_isTaskListRBUTTONUP_Active = true;
				short zDelta = (short) HIWORD(wParam);
				return ::SendMessage(_hSelf, WM_COMMAND, zDelta>0?IDC_PREV_DOC:IDC_NEXT_DOC, 0);
			}
			return TRUE;
		}
		
		case WM_APPCOMMAND :
		{
			switch(GET_APPCOMMAND_LPARAM(lParam))
			{
				case APPCOMMAND_BROWSER_BACKWARD :
				case APPCOMMAND_BROWSER_FORWARD :
					int nbDoc = viewVisible(MAIN_VIEW)?_mainDocTab.nbItem():0;
					nbDoc += viewVisible(SUB_VIEW)?_subDocTab.nbItem():0;
					if (nbDoc > 1)
						activateNextDoc((GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)?dirDown:dirUp);
					_linkTriggered = true;
			}
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case NPPM_GETNBSESSIONFILES :
		{
			const TCHAR *sessionFileName = (const TCHAR *)lParam;
			if ((!sessionFileName) || (sessionFileName[0] == '\0')) return 0;
			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
			{
				return session2Load.nbMainFiles() + session2Load.nbSubFiles();
			}
			return 0;
		}
		
		case NPPM_GETSESSIONFILES :
		{
			const TCHAR *sessionFileName = (const TCHAR *)lParam;
			TCHAR **sessionFileArray = (TCHAR **)wParam;

			if ((!sessionFileName) || (sessionFileName[0] == '\0')) return FALSE;

			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
			{
				size_t i = 0;
				for ( ; i < session2Load.nbMainFiles() ; )
				{
					const TCHAR *pFn = session2Load._mainViewFiles[i]._fileName.c_str();
					lstrcpy(sessionFileArray[i++], pFn);
				}

				for (size_t j = 0 ; j < session2Load.nbSubFiles() ; j++)
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
			Utf8_16_Write     UnicodeConvertor;
			UINT            length  = 0;
			char*            buffer  = NULL;
			ScintillaEditView *pSci;

			if (wParam == MAIN_VIEW)
				pSci = &_mainEditView;
			else if (wParam == SUB_VIEW)
				pSci = &_subEditView;
			else
				return -1;
			

			// get text of current scintilla
			length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

			// convert here      
			UniMode unicodeMode = pSci->getCurrentBuffer()->getUnicodeMode();
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
			Utf8_16_Read    UnicodeConvertor;
			UINT            length  = 0;
			char*            buffer  = NULL;
			ScintillaEditView *pSci;

			if (wParam == MAIN_VIEW)
				pSci = &_mainEditView;
			else if (wParam == SUB_VIEW)
				pSci = &_subEditView;
			else
				return -1;

			// get text of current scintilla
			length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

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

		case NPPM_ACTIVATEDOC :
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
				::SendMessage(_hSelf, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
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
			for (int i = 0 ; verStr[i] ; i++)
			{
				if (verStr[i] == '.')
					isDot = true;
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

			int mainVer, auxVer = 0;
			if (mainVerStr)
				mainVer = generic_atoi(mainVerStr);
			if (auxVerStr)
				auxVer = generic_atoi(auxVerStr);

			return MAKELONG(auxVer, mainVer);
		}

		case WM_ISCURRENTMACRORECORDED :
			return (!_macro.empty() && !_recordingMacro);

		case WM_MACRODLGRUNMACRO:
		{
			if (!_recordingMacro) // if we're not currently recording, then playback the recorded keystrokes
			{
				int times = 1;
				if (_runMacroDlg.getMode() == RM_RUN_MULTI)
				{
					times = _runMacroDlg.getTimes();
				}
				else if (_runMacroDlg.getMode() == RM_RUN_EOF)
				{
					times = -1;
				}
				else
				{
					break;
				}
                          
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
				while (true)
				{
					for (Macro::iterator step = m.begin(); step != m.end(); step++)
						step->PlayBack(this, _pEditView);
						
					counter++;
					if ( times >= 0 )
					{
						if ( counter >= times ) break;
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
							break;
					}
				}
				_pEditView->execute(SCI_ENDUNDOACTION);
			}
		}
		break;

		case NPPM_CREATESCINTILLAHANDLE :
		{
			return (LRESULT)_scintillaCtrls4Plugins.createSintilla((lParam == NULL?_hSelf:(HWND)lParam));
		}
		
		case NPPM_DESTROYSCINTILLAHANDLE :
		{
			return _scintillaCtrls4Plugins.destroyScintilla((HWND)lParam);
		}

		case NPPM_GETNBUSERLANG :
		{
			if (lParam)
				*((int *)lParam) = IDM_LANG_USER;
			return pNppParam->getNbUserLang();
		}

		case NPPM_GETCURRENTDOCINDEX :
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

		case NPPM_SETSTATUSBAR :
		{
			TCHAR *str2set = (TCHAR *)lParam;
			if (!str2set || !str2set[0])
				return FALSE;

			switch (wParam)
			{
				case STATUSBAR_DOC_TYPE :
				case STATUSBAR_DOC_SIZE :
				case STATUSBAR_CUR_POS :
				case STATUSBAR_EOF_FORMAT :
				case STATUSBAR_UNICODE_TYPE :
				case STATUSBAR_TYPING_MODE :
					_statusBar.setText(str2set, wParam);
					return TRUE;
				default :
					return FALSE;
			}
		}

		case NPPM_GETMENUHANDLE :
		{
			if (wParam == NPPPLUGINMENU)
				return (LRESULT)_pluginsManager.getMenuHandle();
			else
				return NULL;
		}

		case NPPM_LOADSESSION :
		{
			fileLoadSession((const TCHAR *)lParam);
			return TRUE;
		}

		case NPPM_SAVECURRENTSESSION :
		{
			return (LRESULT)fileSaveSession(0, NULL, (const TCHAR *)lParam);
		}

		case NPPM_SAVESESSION :
		{
			sessionInfo *pSi = (sessionInfo *)lParam;
			return (LRESULT)fileSaveSession(pSi->nbFile, pSi->files, pSi->sessionFilePathName);
		}

		case NPPM_INTERNAL_CLEARSCINTILLAKEY :
		{
			_mainEditView.execute(SCI_CLEARCMDKEY, wParam);
			_subEditView.execute(SCI_CLEARCMDKEY, wParam);
			return TRUE;		
		}
		case NPPM_INTERNAL_BINDSCINTILLAKEY :
		{
			_mainEditView.execute(SCI_ASSIGNCMDKEY, wParam, lParam);
			_subEditView.execute(SCI_ASSIGNCMDKEY, wParam, lParam);
			
			return TRUE;
		}
		case NPPM_INTERNAL_CMDLIST_MODIFIED :
		{
			//changeMenuShortcut(lParam, (const TCHAR *)wParam);
			::DrawMenuBar(_hSelf);
			return TRUE;
		}

		case NPPM_INTERNAL_MACROLIST_MODIFIED :
		{
			return TRUE;
		}

		case NPPM_INTERNAL_USERCMDLIST_MODIFIED :
		{
			return TRUE;
		}

		case NPPM_INTERNAL_SETCARETWIDTH :
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

		case NPPM_INTERNAL_SETCARETBLINKRATE :
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			_mainEditView.execute(SCI_SETCARETPERIOD, nppGUI._caretBlinkRate);
			_subEditView.execute(SCI_SETCARETPERIOD, nppGUI._caretBlinkRate);
			return TRUE;
		}

		case NPPM_INTERNAL_ISTABBARREDUCED :
		{
			return _toReduceTabBar?TRUE:FALSE;
		}

		// ADD: success->hwnd; failure->NULL
		// REMOVE: success->NULL; failure->hwnd
		case NPPM_MODELESSDIALOG :
		{
			if (wParam == MODELESSDIALOGADD)
			{
				for (size_t i = 0 ; i < _hModelessDlgs.size() ; i++)
					if (_hModelessDlgs[i] == (HWND)lParam)
						return NULL;
				_hModelessDlgs.push_back((HWND)lParam);
				return lParam;
			}
			else if (wParam == MODELESSDIALOGREMOVE)
			{
				for (size_t i = 0 ; i < _hModelessDlgs.size() ; i++)
					if (_hModelessDlgs[i] == (HWND)lParam)
					{
						vector<HWND>::iterator hDlg = _hModelessDlgs.begin() + i;
						_hModelessDlgs.erase(hDlg);
						return NULL;
					}
				return lParam;
			}
			return TRUE;
		}

		case WM_CONTEXTMENU :
		{
			if (pNppParam->_isTaskListRBUTTONUP_Active)
			{
				pNppParam->_isTaskListRBUTTONUP_Active = false;
			}
			else
			{
				if ((HWND(wParam) == _mainEditView.getHSelf()) || (HWND(wParam) == _subEditView.getHSelf()))
				{
					if ((HWND(wParam) == _mainEditView.getHSelf())) {
						switchEditViewTo(MAIN_VIEW);
					} else {
						switchEditViewTo(SUB_VIEW);
					}
					POINT p;
					::GetCursorPos(&p);
					ContextMenu scintillaContextmenu;
					vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
					vector<bool> isEnable;
					for (size_t i = 0 ; i < tmp.size() ; i++)
					{
						isEnable.push_back((::GetMenuState(_mainMenuHandle, tmp[i]._cmdID, MF_BYCOMMAND)&MF_DISABLED) == 0);
					}
					scintillaContextmenu.create(_hSelf, tmp);
					for (size_t i = 0 ; i < isEnable.size() ; i++)
						scintillaContextmenu.enableItem(tmp[i]._cmdID, isEnable[i]);

					scintillaContextmenu.display(p);
					return TRUE;
				}
			}

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case WM_NOTIFY:
		{
			checkClipboard();
			checkUndoState();
			checkMacroState();
			_pluginsManager.notify(reinterpret_cast<SCNotification *>(lParam));
			return notify(reinterpret_cast<SCNotification *>(lParam));
		}

		case NPPM_CHECKDOCSTATUS :
		case WM_ACTIVATEAPP :
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

		case NPPM_GETCHECKDOCOPT :
		{
			return (LRESULT)((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection;
		}

		case NPPM_SETCHECKDOCOPT :
		{
			// If nothing is changed by user, then we allow to set this value
			if (((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection == ((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetectionOriginalValue)
				((NppGUI &)(pNppParam->getNppGUI()))._fileAutoDetection = (ChangeDetect)wParam;
			return TRUE;
		}

		case NPPM_GETPOSFROMBUFFERID :
		{
			int i;

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
			return -1;
		}

		case NPPM_GETFULLPATHFROMBUFFERID :
		{
			return MainFileManager->getFileNameFromBuffer((BufferID)wParam, (TCHAR *)lParam);
		}
		
		case NPPM_ENABLECHECKDOCOPT:
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
		
		case WM_ACTIVATE :
			_pEditView->getFocus();
			return TRUE;

		case WM_DROPFILES:
		{
			dropFiles(reinterpret_cast<HDROP>(wParam));
			return TRUE;
		}

		case WM_UPDATESCINTILLAS:
		{
			//reset styler for change in Stylers.xml
			_mainEditView.defineDocType(_mainEditView.getCurrentBuffer()->getLangType());
			_subEditView.defineDocType(_subEditView.getCurrentBuffer()->getLangType());
			_mainEditView.performGlobalStyles();
			_subEditView.performGlobalStyles();

			drawTabbarColoursFromStylerArray();

			// Notify plugins of update to styles xml
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_WORDSTYLESUPDATED;
			scnN.nmhdr.hwndFrom = _hSelf;
			scnN.nmhdr.idFrom = (uptr_t) _pEditView->getCurrentBufferID();
			_pluginsManager.notify(&scnN);
			return TRUE;
		}

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
		{
			const NppGUI & nppgui = pNppParam->getNppGUI();
			
			Session currentSession;
			if (nppgui._rememberLastSession) 
			{
				getCurrentOpenedFiles(currentSession);
				//Lock the recent file list so it isnt populated with opened files
				//Causing them to show on restart even though they are loaded by session
				_lastRecentFileList.setLock(true);	//only lock when the session is remembered
			}
			
			bool allClosed = fileCloseAll();	//try closing files before doing anything else
			
			if (nppgui._rememberLastSession) 
			{
				_lastRecentFileList.setLock(false);	//only lock when the session is remembered
			}
			
			if (!allClosed) 
			{
				//User cancelled the shutdown
				return FALSE;
			}

			if (_isfullScreen)	//closing, return to windowed mode
				fullScreenToggle();

			if (_beforePostIt.isPostIt)
				::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_POSTIT, 0);

			if (_configStyleDlg.isCreated() && ::IsWindowVisible(_configStyleDlg.getHSelf()))
				_configStyleDlg.restoreGlobalOverrideValues();
			
			SCNotification scnN;
			scnN.nmhdr.code = NPPN_SHUTDOWN;
			scnN.nmhdr.hwndFrom = _hSelf;
			scnN.nmhdr.idFrom = 0;
			_pluginsManager.notify(&scnN);

			saveFindHistory();

			_lastRecentFileList.saveLRFL();
			saveScintillaParams(SCIV_PRIMARY);
			saveScintillaParams(SCIV_SECOND);
			saveGUIParams();
			saveUserDefineLangs();
			saveShortcuts();
			if (nppgui._rememberLastSession)
				saveSession(currentSession);

			//Sends WM_DESTROY, Notepad++ will end
			::DestroyWindow(hwnd);

			return TRUE;
		}

		case WM_DESTROY:
		{	
			killAllChildren();	
			::PostQuitMessage(0);
			gNppHWND = NULL;
			return TRUE;
		}

		case WM_SYSCOMMAND:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if ((nppgui._isMinimizedToTray) && (wParam == SC_MINIMIZE))
			{
				if (!_pTrayIco)
					_pTrayIco = new trayIconControler(_hSelf, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)), TEXT(""));

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
			::SendMessage(_hSelf, WM_COMMAND, IDM_FILE_NEW, 0);
			return TRUE;
		}

		case IDC_MINIMIZED_TRAY:
		{
			switch (lParam)
			{
				//case WM_LBUTTONDBLCLK:
				case WM_LBUTTONUP :
					_pEditView->getFocus();
					::ShowWindow(_hSelf, SW_SHOW);
					_pTrayIco->doTrayIcon(REMOVE);
					::SendMessage(_hSelf, WM_SIZE, 0, 0);
					return TRUE;
/*
				case WM_RBUTTONUP:
				{
					POINT p;
					GetCursorPos(&p);
					TrackPopupMenu(hTrayIconMenu, TPM_LEFTALIGN, p.x, p.y, 0, hwnd, NULL);
					return TRUE; 
				}
*/
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
			if (!lParam) return NULL;

			TCHAR *moduleName = (TCHAR *)lParam;
			TCHAR *windowName = (TCHAR *)wParam;
			vector<DockingCont *> dockContainer = _dockingManager.getContainerInfo();
			for (size_t i = 0 ; i < dockContainer.size() ; i++)
			{
				vector<tTbData *> tbData = dockContainer[i]->getDataOfAllTb();
				for (size_t j = 0 ; j < tbData.size() ; j++)
				{
					if (generic_stricmp(moduleName, tbData[j]->pszModuleName) == 0)
					{
						if (!windowName)
							return (LRESULT)tbData[j]->hClient;
						else if (generic_stricmp(windowName, tbData[j]->pszName) == 0)
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
			return _winVersion;
		}

		case NPPM_MAKECURRENTBUFFERDIRTY :
		{
			_pEditView->getCurrentBuffer()->setDirty(true);
			return TRUE;
		}

		case NPPM_GETENABLETHEMETEXTUREFUNC :
		{
			return (LRESULT)pNppParam->getEnableThemeDlgTexture();
		}

		case NPPM_GETPLUGINSCONFIGDIR :
		{
			if (!lParam || !wParam)
				return FALSE;

			const TCHAR *pluginsConfigDirPrefix = pNppParam->getAppDataNppDir();
			
			if (!pluginsConfigDirPrefix[0])
				pluginsConfigDirPrefix = pNppParam->getNppPath();

			const TCHAR *secondPart = TEXT("plugins\\Config");
			
			int len = wParam;
			if (len < lstrlen(pluginsConfigDirPrefix) + lstrlen(secondPart))
				return FALSE;

			TCHAR *pluginsConfigDir = (TCHAR *)lParam;			
			lstrcpy(pluginsConfigDir, pluginsConfigDirPrefix);

			::PathAppend(pluginsConfigDir, secondPart);
			return TRUE;
		}

		case NPPM_MSGTOPLUGIN :
		{
			return _pluginsManager.relayPluginMessages(Message, wParam, lParam);
		}

		case NPPM_HIDETABBAR :
		{
			bool hide = (lParam != 0);
			bool oldVal = DocTabView::getHideTabBarStatus();
			if (hide == oldVal) return oldVal;

			DocTabView::setHideTabBarStatus(hide);
			::SendMessage(_hSelf, WM_SIZE, 0, 0);

			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			if (hide)
				nppGUI._tabStatus |= TAB_HIDE;
			else
				nppGUI._tabStatus &= ~TAB_HIDE;

			return oldVal;
		}
		case NPPM_ISTABBARHIDDEN :
		{
			return _mainDocTab.getHideTabBarStatus();
		}


		case NPPM_HIDETOOLBAR :
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

		case NPPM_HIDEMENU :
		{
			bool hide = (lParam == TRUE);
			bool isHidden = ::GetMenu(_hSelf) == NULL;
			if (hide == isHidden)
				return isHidden;

			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			nppGUI._menuBarShow = !hide;
			if (nppGUI._menuBarShow)
				::SetMenu(_hSelf, _mainMenuHandle);
			else
				::SetMenu(_hSelf, NULL);

			return isHidden;
		}
		case NPPM_ISMENUHIDDEN :
		{
			return (::GetMenu(_hSelf) == NULL);
		}

		case NPPM_HIDESTATUSBAR:
		{
			bool show = (lParam != TRUE);
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			bool oldVal = nppGUI._statusBarShow;
			if (show == oldVal)
			{
				return oldVal;
			}
            RECT rc;
			getClientRect(rc);
			
			nppGUI._statusBarShow = show;
            _statusBar.display(nppGUI._statusBarShow);
            ::SendMessage(_hSelf, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.bottom, rc.right));
            return oldVal;
        }

		case NPPM_ISSTATUSBARHIDDEN :
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			return !nppGUI._statusBarShow;
		}

/*
		case NPPM_ADDREBAR :
		{
			if (!lParam)
				return FALSE;
			_rebarTop.addBand((REBARBANDINFO*)lParam, false);
			return TRUE;
		}

		case NPPM_UPDATEREBAR :
		{
			if (!lParam || wParam < REBAR_BAR_EXTERNAL)
				return FALSE;
			_rebarTop.reNew((int)wParam, (REBARBANDINFO*)lParam);
			return TRUE;
		}

		case NPPM_REMOVEREBAR :
		{
			if (wParam < REBAR_BAR_EXTERNAL)
				return FALSE;
			_rebarTop.removeBand((int)wParam);
			return TRUE;
		}
*/
		case NPPM_INTERNAL_ISFOCUSEDTAB :
		{
			HWND hTabToTest = (currentView() == MAIN_VIEW)?_mainDocTab.getHSelf():_subDocTab.getHSelf();
			return (HWND)lParam == hTabToTest;
		}

		case NPPM_INTERNAL_GETMENU :
		{
			return (LRESULT)_mainMenuHandle;
		}
	
		case NPPM_INTERNAL_CLEARINDICATOR :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_FOUND_STYLE_2);
			return TRUE;
		}
		case NPPM_INTERNAL_CLEARINDICATORTAGMATCH :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGMATCH);
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGATTR);
			return TRUE;
		}
		case NPPM_INTERNAL_CLEARINDICATORTAGATTR :
		{
			_pEditView->clearIndicator(SCE_UNIVERSAL_TAGATTR);
			return TRUE;
		}

		case NPPM_INTERNAL_SWITCHVIEWFROMHWND :
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

		case NPPM_INTERNAL_UPDATETITLEBAR :
		{
			setTitle();
			return TRUE;
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
				::SetMenu(_hSelf, _mainMenuHandle);
				
			return TRUE;
		}

		case WM_EXITMENULOOP:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if (!nppgui._menuBarShow && !wParam && !_sysMenuEntering)
				::SetMenu(_hSelf, NULL);
			_sysMenuEntering = false;
			return FALSE;
		}

		default:
		{
			if (Message == WDN_NOTIFY)
			{
				NMWINDLG* nmdlg = (NMWINDLG*)lParam;
				switch (nmdlg->type)
				{
					case WDT_ACTIVATE:
						activateDoc(nmdlg->curSel);
						nmdlg->processed = TRUE;
						break;
					case WDT_SAVE:
					{
						//loop through nmdlg->nItems, get index and save it
						for (int i = 0; i < (int)nmdlg->nItems; i++) {
							fileSave(_pDocTab->getBufferByIndex(i));
						}
						nmdlg->processed = TRUE;
					}
					break;
					case WDT_CLOSE:
					{
						//loop through nmdlg->nItems, get index and close it
						for (int i = 0; i < (int)nmdlg->nItems; i++) {
							fileClose(_pDocTab->getBufferByIndex(nmdlg->Items[i]), currentView());
							UINT pos = nmdlg->Items[i];
							nmdlg->Items[i] = 0xFFFFFFFF; // indicate file was closed
							
							for (int j=i+1; j<(int)nmdlg->nItems; ++j)
								if (nmdlg->Items[j] > pos) 
									--nmdlg->Items[j];
						}
						nmdlg->processed = TRUE;
					}
					break;
					case WDT_SORT:
						if (nmdlg->nItems != _pDocTab->nbItem())	//sanity check, if mismatch just abort
							break;
						//Collect all buffers
						std::vector<BufferID> tempBufs;
						for(int i = 0; i < (int)nmdlg->nItems; i++) {
							tempBufs.push_back(_pDocTab->getBufferByIndex(i));
						}
						//Reset buffers
						for(int i = 0; i < (int)nmdlg->nItems; i++) {
							_pDocTab->setBuffer(i, tempBufs[nmdlg->Items[i]]);
						}
						activateBuffer(_pDocTab->getBufferByIndex(_pDocTab->getCurrentTabIndex()), currentView());
						break;
				}
				return TRUE;
			}

			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}
	}

	_pluginsManager.relayNppMessages(Message, wParam, lParam);
	return result;
}

LRESULT CALLBACK Notepad_plus::Notepad_plus_Proc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{

  static bool isFirstGetMinMaxInfoMsg = true;

  switch(Message)
  {
    case WM_NCCREATE : // First message we get the ptr of instantiated object
                       // then stock it into GWL_USERDATA index in order to retrieve afterward
	{
		Notepad_plus *pM30ide = (Notepad_plus *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		pM30ide->_hSelf = hwnd;
		::SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)pM30ide);

		return TRUE;
	}

    default :
    {
      return ((Notepad_plus *)::GetWindowLongPtr(hwnd, GWL_USERDATA))->runProc(hwnd, Message, wParam, lParam);
    }
  }
}

void Notepad_plus::fullScreenToggle()
{
	RECT fullscreenArea;		//RECT used to calculate window fullscrene size

	_isfullScreen = !_isfullScreen;
	if (_isfullScreen)
	{
		_winPlace.length = sizeof(_winPlace);
		::GetWindowPlacement(_hSelf, &_winPlace);

		//Preset view area, in case something fails, primary monitor values
		fullscreenArea.top = 0;
		fullscreenArea.left = 0;
		fullscreenArea.right = GetSystemMetrics(SM_CXSCREEN);	
		fullscreenArea.bottom = GetSystemMetrics(SM_CYSCREEN);
		
		//if (_winVersion != WV_NT)
		{
			HMONITOR currentMonitor;	//Handle to monitor where fullscreen should go
			MONITORINFO mi;				//Info of that monitor
			//Caution, this will not work on windows 95, so probably add some checking of some sorts like Unicode checks, IF 95 were to be supported
			currentMonitor = MonitorFromWindow(_hSelf, MONITOR_DEFAULTTONEAREST);	//should always be valid monitor handle
			mi.cbSize = sizeof(MONITORINFO);
			if (GetMonitorInfo(currentMonitor, &mi) != FALSE)
			{
				fullscreenArea = mi.rcMonitor;
				fullscreenArea.right -= fullscreenArea.left;
				fullscreenArea.bottom -= fullscreenArea.top;
			}
		}
		//Hide menu
		::SetMenu(_hSelf, NULL);

		//Hide window so windows can properly update it
		::ShowWindow(_hSelf, SW_HIDE);

		//Hide rebar
		_rebarTop.display(false);
		_rebarBottom.display(false);

		//Set popup style for fullscreen window and store the old style
		_prevStyles = ::SetWindowLongPtr( _hSelf, GWL_STYLE, WS_POPUP );
		if (!_prevStyles) {	//something went wrong, use default settings
			_prevStyles = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
		}
		
		//Set topmost window, show the window and redraw it
		::SetWindowPos(_hSelf, HWND_TOPMOST, fullscreenArea.left, fullscreenArea.top, fullscreenArea.right, fullscreenArea.bottom, SWP_NOZORDER);
		::ShowWindow(_hSelf, SW_SHOW);
		::SendMessage(_hSelf, WM_SIZE, 0, 0);
	}
	else
	{
		//Hide window for updating, restore style and menu then restore position and Z-Order
		::ShowWindow(_hSelf, SW_HIDE);

		NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
		if (nppGUI._menuBarShow)
			::SetMenu(_hSelf, _mainMenuHandle);

		::SetWindowLongPtr( _hSelf, GWL_STYLE, _prevStyles);
		::SetWindowPos(_hSelf, HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW|SWP_NOZORDER);

		//Show rebar
		_rebarTop.display(true);
		_rebarBottom.display(true);

		if (_winPlace.length)
		{
			if (_winPlace.showCmd == SW_SHOWMAXIMIZED)
			{
				::ShowWindow(_hSelf, SW_RESTORE);
				::ShowWindow(_hSelf, SW_SHOWMAXIMIZED);
			}
			else
			{
				::SetWindowPlacement(_hSelf, &_winPlace);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
		}
		else	//fallback
		{
			::ShowWindow(_hSelf, SW_SHOW);
		}
	}
	::SetForegroundWindow(_hSelf);
}


void Notepad_plus::doSynScorll(HWND whichView)
{
	int column = 0;
	int line = 0;
	ScintillaEditView *pView;

	// var for Line
	int mainCurrentLine, subCurrentLine;

	// var for Column
	int mxoffset, sxoffset;
	int pixel;
	int mainColumn, subColumn;

    if (whichView == _mainEditView.getHSelf())
	{	
		if (_syncInfo._isSynScollV)
		{
			// Compute for Line
			mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
			subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
			line = mainCurrentLine - _syncInfo._line - subCurrentLine;
		}
		if (_syncInfo._isSynScollH)
		{
			// Compute for Column
			mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
			pixel = int(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			mainColumn = mxoffset/pixel;

			sxoffset = _subEditView.execute(SCI_GETXOFFSET);
			pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			subColumn = sxoffset/pixel;
			column = mainColumn - _syncInfo._column - subColumn;
		}
		pView = &_subEditView;
    }
    else if (whichView == _subEditView.getHSelf())
    {
		if (_syncInfo._isSynScollV)
		{
			// Compute for Line
			mainCurrentLine = _mainEditView.execute(SCI_GETFIRSTVISIBLELINE);
			subCurrentLine = _subEditView.execute(SCI_GETFIRSTVISIBLELINE);
			line = subCurrentLine + _syncInfo._line - mainCurrentLine;
		}
		if (_syncInfo._isSynScollH)
		{
			// Compute for Column
			mxoffset = _mainEditView.execute(SCI_GETXOFFSET);
			pixel = int(_mainEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			mainColumn = mxoffset/pixel;

			sxoffset = _subEditView.execute(SCI_GETXOFFSET);
			pixel = int(_subEditView.execute(SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM)"P"));
			subColumn = sxoffset/pixel;
			column = subColumn + _syncInfo._column - mainColumn;
		}
		pView = &_mainEditView;
    }
    else
        return;

	pView->scroll(column, line);
}

bool Notepad_plus::getIntegralDockingData(tTbData & dockData, int & iCont, bool & isVisible)
{
	DockingManagerData & dockingData = (DockingManagerData &)(NppParameters::getInstance())->getNppGUI()._dockingData;

	for (size_t i = 0 ; i < dockingData._pluginDockInfo.size() ; i++)
	{
		const PlugingDlgDockingInfo & pddi = dockingData._pluginDockInfo[i];

		if (!generic_stricmp(pddi._name, dockData.pszModuleName) && (pddi._internalID == dockData.dlgID))
		{
			iCont				= pddi._currContainer;
			isVisible			= pddi._isVisible;
			dockData.iPrevCont	= pddi._prevContainer;

			if (dockData.iPrevCont != -1)
			{
				int cont = (pddi._currContainer < DOCKCONT_MAX ? pddi._prevContainer : pddi._currContainer);
				RECT *pRc = dockingData.getFloatingRCFrom(cont);
				if (pRc)
					dockData.rcFloat	= *pRc;
			}
			return true;
		}
	}
	return false;
}


void Notepad_plus::getCurrentOpenedFiles(Session & session)
{
	_mainEditView.saveCurrentPos();	//save position so itll be correct in the session
	_subEditView.saveCurrentPos();	//both views
	session._activeView = currentView();
	session._activeMainIndex = _mainDocTab.getCurrentTabIndex();
	session._activeSubIndex = _subDocTab.getCurrentTabIndex();

	//Use _invisibleEditView to temporarily open documents to retrieve markers
	//Buffer * mainBuf = _mainEditView.getCurrentBuffer();
	//Buffer * subBuf = _subEditView.getCurrentBuffer();
	Document oldDoc = _invisibleEditView.execute(SCI_GETDOCPOINTER);

	for (int i = 0 ; i < _mainDocTab.nbItem() ; i++)
	{
		BufferID bufID = _mainDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(bufID);
		if (!buf->isUntitled() && PathFileExists(buf->getFullPathName()))
		{
			generic_string	languageName	= getLangFromMenu( buf );
			const TCHAR *langName	= languageName.c_str();

			sessionFileInfo sfi(buf->getFullPathName(), langName, buf->getPosition(&_mainEditView));

			//_mainEditView.activateBuffer(buf->getID());
			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			int maxLine = _invisibleEditView.execute(SCI_GETLINECOUNT);
			for (int j = 0 ; j < maxLine ; j++)
			{
				if ((_invisibleEditView.execute(SCI_MARKERGET, j)&(1 << MARK_BOOKMARK)) != 0)
				{
					sfi.marks.push_back(j);
				}
			}
			session._mainViewFiles.push_back(sfi);
		}
	}

	for (int i = 0 ; i < _subDocTab.nbItem() ; i++)
	{
		BufferID bufID = _subDocTab.getBufferByIndex(i);
		Buffer * buf = MainFileManager->getBufferByID(bufID);
		if (!buf->isUntitled() && PathFileExists(buf->getFullPathName()))
		{
			generic_string	languageName	= getLangFromMenu( buf );
			const TCHAR *langName	= languageName.c_str();

			sessionFileInfo sfi(buf->getFullPathName(), langName, buf->getPosition(&_subEditView));

			_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, buf->getDocument());
			int maxLine = _invisibleEditView.execute(SCI_GETLINECOUNT);
			for (int j = 0 ; j < maxLine ; j++)
			{
				if ((_invisibleEditView.execute(SCI_MARKERGET, j)&(1 << MARK_BOOKMARK)) != 0)
				{
					sfi.marks.push_back(j);
				}
			}
			session._subViewFiles.push_back(sfi);
		}
	}

	//_mainEditView.activateBuffer(mainBuf->getID());	//restore buffer
	//_subEditView.activateBuffer(subBuf->getID());	//restore buffer
	//_mainEditView.execute(SCI_SETDOCPOINTER, 0, mainBuf->getDocument());
	//_mainEditView.restoreCurrentPos();
	//_subEditView.execute(SCI_SETDOCPOINTER, 0, subBuf->getDocument());
	//_subEditView.restoreCurrentPos();

	_invisibleEditView.execute(SCI_SETDOCPOINTER, 0, oldDoc);
}

bool Notepad_plus::fileLoadSession(const TCHAR *fn)
{
	bool result = false;
	const TCHAR *sessionFileName = NULL;
	if (fn == NULL)
	{
		FileDialog fDlg(_hSelf, _hInst);
		fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
		const TCHAR *ext = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();
		generic_string sessionExt = TEXT("");
		if (*ext != '\0')
		{
			if (*ext != '.') 
				sessionExt += TEXT(".");
			sessionExt += ext;
			fDlg.setExtFilter(TEXT("Session file"), sessionExt.c_str(), NULL);
		}
		sessionFileName = fDlg.doOpenSingleFileDlg();
	}
	else
	{
		if (PathFileExists(fn))
			sessionFileName = fn;
	}
	
	if (sessionFileName)
	{
		bool isAllSuccessful = true;
		Session session2Load;

		if ((NppParameters::getInstance())->loadSession(session2Load, sessionFileName))
		{
			isAllSuccessful = loadSession(session2Load);
			result = true;
		}
		if (!isAllSuccessful)
			(NppParameters::getInstance())->writeSession(session2Load, sessionFileName);
	}
	return result;
}


const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames, const TCHAR *sessionFile2save)
{
	if (sessionFile2save)
	{
		Session currentSession;
		if ((nbFile) && (!fileNames))
		{

			for (size_t i = 0 ; i < nbFile ; i++)
			{
				if (PathFileExists(fileNames[i]))
					currentSession._mainViewFiles.push_back(generic_string(fileNames[i]));
			}
		}
		else
			getCurrentOpenedFiles(currentSession);

		(NppParameters::getInstance())->writeSession(currentSession, sessionFile2save);
		return sessionFile2save;
	}
	return NULL;
}

const TCHAR * Notepad_plus::fileSaveSession(size_t nbFile, TCHAR ** fileNames)
{
	const TCHAR *sessionFileName = NULL;
	
	FileDialog fDlg(_hSelf, _hInst);
	const TCHAR *ext = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();

	fDlg.setExtFilter(TEXT("All types"), TEXT(".*"), NULL);
	generic_string sessionExt = TEXT("");
	if (*ext != '\0')
	{
		if (*ext != '.') 
			sessionExt += TEXT(".");
		sessionExt += ext;
		fDlg.setExtFilter(TEXT("Session file"), sessionExt.c_str(), NULL);
	}
	sessionFileName = fDlg.doSaveDlg();

	return fileSaveSession(nbFile, fileNames, sessionFileName);
}


bool Notepad_plus::str2Cliboard(const TCHAR *str2cpy)
{
	if (!str2cpy)
		return false;

	int len2Allocate = lstrlen(str2cpy) + 1;
	len2Allocate *= sizeof(TCHAR);
	unsigned int cilpboardFormat = CF_TEXT;

#ifdef UNICODE
	cilpboardFormat = CF_UNICODETEXT;
#endif

	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, len2Allocate);
	if (hglbCopy == NULL) 
	{ 
		return false; 
	} 
	
	if (!::OpenClipboard(_hSelf)) 
		return false; 
		
	::EmptyClipboard();

	// Lock the handle and copy the text to the buffer. 
	TCHAR *pStr = (TCHAR *)::GlobalLock(hglbCopy);
	lstrcpy(pStr, str2cpy);
	::GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
	::SetClipboardData(cilpboardFormat, hglbCopy);
	::CloseClipboard();
	return true;
}

//ONLY CALL IN CASE OF EMERGENCY: EXCEPTION
//This function is destructive
bool Notepad_plus::emergency(generic_string emergencySavedDir) 
{
	bool filestatus = false;
	bool dumpstatus = false;
	do {
		if (::CreateDirectory(emergencySavedDir.c_str(), NULL) == FALSE && ::GetLastError() != ERROR_ALREADY_EXISTS) {
			break;
		}

		filestatus = dumpFiles(emergencySavedDir.c_str(), TEXT("File"));

	} while (false);

	bool status = filestatus;// && dumpstatus;
	return status;
}

bool Notepad_plus::dumpFiles(const TCHAR * outdir, const TCHAR * fileprefix) {
	//start dumping unsaved files to recovery directory
	bool somethingsaved = false;
	bool somedirty = false;
	TCHAR savePath[MAX_PATH] = {0};

	//rescue primary
	for(int i = 0; i < MainFileManager->getNrBuffers(); i++) {
		Buffer * docbuf = MainFileManager->getBufferByIndex(i);
		if (!docbuf->isDirty())	//skip saved documents
			continue;
		else
			somedirty = true;

		const TCHAR * unitext = (docbuf->getUnicodeMode() != uni8Bit)?TEXT("_utf8"):TEXT("");
		wsprintf(savePath, TEXT("%s\\%s%03d%s.dump"), outdir, fileprefix, i, unitext);

		bool res = MainFileManager->saveBuffer(docbuf->getID(), savePath);

		somethingsaved |= res;
	}

	return somethingsaved || !somedirty;
}

void Notepad_plus::drawTabbarColoursFromStylerArray()
{
	Style *stActText = getStyleFromName(TABBAR_ACTIVETEXT);
	if (stActText && stActText->_fgColor != -1)
		TabBarPlus::setColour(stActText->_fgColor, TabBarPlus::activeText);

	Style *stActfocusTop = getStyleFromName(TABBAR_ACTIVEFOCUSEDINDCATOR);
	if (stActfocusTop && stActfocusTop->_fgColor != -1)
		TabBarPlus::setColour(stActfocusTop->_fgColor, TabBarPlus::activeFocusedTop);

	Style *stActunfocusTop = getStyleFromName(TABBAR_ACTIVEUNFOCUSEDINDCATOR);
	if (stActunfocusTop && stActunfocusTop->_fgColor != -1)
		TabBarPlus::setColour(stActunfocusTop->_fgColor, TabBarPlus::activeUnfocusedTop);

	Style *stInact = getStyleFromName(TABBAR_INACTIVETEXT);
	if (stInact && stInact->_fgColor != -1)
		TabBarPlus::setColour(stInact->_fgColor, TabBarPlus::inactiveText);
	if (stInact && stInact->_bgColor != -1)
		TabBarPlus::setColour(stInact->_bgColor, TabBarPlus::inactiveBg);
}

void Notepad_plus::notifyBufferChanged(Buffer * buffer, int mask) {
	NppParameters *pNppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParam->getNppGUI();

	_mainEditView.bufferUpdated(buffer, mask);
	_subEditView.bufferUpdated(buffer, mask);
	_mainDocTab.bufferUpdated(buffer, mask);
	_subDocTab.bufferUpdated(buffer, mask);

	bool mainActive = (_mainEditView.getCurrentBuffer() == buffer);
	bool subActive = (_subEditView.getCurrentBuffer() == buffer);

	//Only event that applies to non-active Buffers
	if (mask & BufferChangeStatus) {	//reload etc
		bool didDialog = false;
		switch(buffer->getStatus()) {
			case DOC_UNNAMED: 	//nothing todo
			{
				break;
			}
			case DOC_REGULAR: 	//nothing todo
			{
				break; 
			}
			case DOC_MODIFIED:	//ask for reloading
			{
				bool autoUpdate = (nppGUI._fileAutoDetection == cdAutoUpdate) || (nppGUI._fileAutoDetection == cdAutoUpdateGo2end);
				if (!autoUpdate || buffer->isDirty())
				{
					didDialog = true;
					if (doReloadOrNot(buffer->getFullPathName(), buffer->isDirty()) != IDYES)
						break;	//abort
				}
				//activateBuffer(buffer->getID(), iView);	//activate the buffer in the first view possible
				doReload(buffer->getID(), false);
				if (mainActive || subActive)
				{
					performPostReload(mainActive?MAIN_VIEW:SUB_VIEW);
				}
				break;
			}
			case DOC_DELETED: 	//ask for keep
			{
				int index = _pDocTab->getIndexByBuffer(buffer->getID());
				int iView = currentView();
				if (index == -1)
					iView = otherView();
				//activateBuffer(buffer->getID(), iView);	//activate the buffer in the first view possible
				didDialog = true;
				if (doCloseOrNot(buffer->getFullPathName()) == IDNO)
				{
					//close in both views, doing current view last since that has to remain opened
					doClose(buffer->getID(), otherView());
					doClose(buffer->getID(), currentView());
				}
				break;
			}
		}

		if (didDialog)
		{
			int curPos = _pEditView->execute(SCI_GETCURRENTPOS);
			::PostMessage(_pEditView->getHSelf(), WM_LBUTTONUP, 0, 0);
			::PostMessage(_pEditView->getHSelf(), SCI_SETSEL, curPos, curPos);
		}
	}

	if (!mainActive && !subActive)
	{
		return;
	}

	if (mask & (BufferChangeLanguage))
	{
		if (mainActive)
			_autoCompleteMain.setLanguage(buffer->getLangType());
		if (subActive)
			_autoCompleteSub.setLanguage(buffer->getLangType());
	}

	if ((currentView() == MAIN_VIEW) && !mainActive)
		return;

	if ((currentView() == SUB_VIEW) && !subActive)
		return;

	if (mask & (BufferChangeDirty|BufferChangeFilename)) 
	{
		checkDocState();
		setTitle();
		TCHAR dir[MAX_PATH];
		lstrcpy(dir, buffer->getFullPathName());
		PathRemoveFileSpec(dir);
		setWorkingDir(dir);
	}

	if (mask & (BufferChangeLanguage)) 
	{
		checkLangsMenu(-1);	//let N++ do search for the item
		setLangStatus(buffer->getLangType());
		if (_mainEditView.getCurrentBuffer() == buffer)
			_autoCompleteMain.setLanguage(buffer->getLangType());
		else if (_subEditView.getCurrentBuffer() == buffer)
			_autoCompleteSub.setLanguage(buffer->getLangType());
		
		SCNotification scnN;
		scnN.nmhdr.code = NPPN_LANGCHANGED;
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = (uptr_t)_pEditView->getCurrentBufferID();
		_pluginsManager.notify(&scnN);
	}

	if (mask & (BufferChangeFormat|BufferChangeLanguage|BufferChangeUnicode))
	{
		updateStatusBar();
		checkUnicodeMenuItems(buffer->getUnicodeMode());
		setUniModeText(buffer->getUnicodeMode());
		setDisplayFormat(buffer->getFormat());
		enableConvertMenuItems(buffer->getFormat());
	}

	if (mask & (BufferChangeReadonly))
	{
		checkDocState();
	}
}

void Notepad_plus::notifyBufferActivated(BufferID bufid, int view) {
	Buffer * buf = MainFileManager->getBufferByID(bufid);
	buf->increaseRecentTag();
	
	if (view == MAIN_VIEW) {
		_autoCompleteMain.setLanguage(buf->getLangType());
	} else if (view == SUB_VIEW) {
		_autoCompleteSub.setLanguage(buf->getLangType());
	}

	if (view != currentView()) {
		return;	//dont care if another view did something
	}

	checkDocState();
	dynamicCheckMenuAndTB();
	setLangStatus(buf->getLangType());
	updateStatusBar();
	checkUnicodeMenuItems(buf->getUnicodeMode());
	setUniModeText(buf->getUnicodeMode());
	setDisplayFormat(buf->getFormat());
	enableConvertMenuItems(buf->getFormat());
	TCHAR dir[MAX_PATH];
	lstrcpy(dir, buf->getFullPathName());
	PathRemoveFileSpec(dir);
	setWorkingDir(dir);
	setTitle();
	//Make sure the colors of the tab controls match
	::InvalidateRect(_mainDocTab.getHSelf(), NULL, FALSE);
	::InvalidateRect(_subDocTab.getHSelf(), NULL, FALSE);

	SCNotification scnN;
	scnN.nmhdr.code = NPPN_BUFFERACTIVATED;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = (uptr_t)bufid;
	_pluginsManager.notify(&scnN);

	_linkTriggered = true;
}

void Notepad_plus::loadCommandlineParams(const TCHAR * commandLine, CmdLineParams * pCmdParams) {
	if (!commandLine || ! pCmdParams)
		return;

	FileNameStringSplitter fnss(commandLine);
	const TCHAR *pFn = NULL;
			
 	LangType lt = pCmdParams->_langType;//LangType(pCopyData->dwData & LASTBYTEMASK);
	int ln =  pCmdParams->_line2go;
	bool readOnly = pCmdParams->_isReadOnly;

	BufferID lastOpened = BUFFER_INVALID;
	for (int i = 0 ; i < fnss.size() ; i++)
	{
		pFn = fnss.getFileName(i);
		BufferID bufID = doOpen(pFn, readOnly);
		if (bufID == BUFFER_INVALID)	//cannot open file
			continue;

		lastOpened = bufID;

		if (lt != L_EXTERNAL) 
		{
			Buffer * pBuf = MainFileManager->getBufferByID(bufID);
			pBuf->setLangType(lt);
		}

		if (ln != -1) 
		{	//we have to move the cursor manually
			int iView = currentView();	//store view since fileswitch can cause it to change
			switchToFile(bufID);	//switch to the file. No deferred loading, but this way we can easily move the cursor to the right position

			_pEditView->execute(SCI_GOTOLINE, ln-1);
			switchEditViewTo(iView);	//restore view
		}
	}
	if (lastOpened != BUFFER_INVALID) {
		switchToFile(lastOpened);
	}
}
