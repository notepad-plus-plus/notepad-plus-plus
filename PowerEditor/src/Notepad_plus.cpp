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
#include "Notepad_plus.h"
#include "SysMsg.h"
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
#include <time.h>
#include <algorithm>

const char Notepad_plus::_className[32] = NOTEPAD_PP_CLASS_NAME;
const char *urlHttpRegExpr = "http://[a-z0-9_\\-\\+.:?&@=/%#]*";

int docTabIconIDs[] = {IDI_SAVED_ICON, IDI_UNSAVED_ICON, IDI_READONLY_ICON};
enum tb_stat {tb_saved, tb_unsaved, tb_ro};

struct SortTaskListPred
{
	ScintillaEditView *_views[2];

	SortTaskListPred(ScintillaEditView &p, ScintillaEditView &s)
	{
		_views[MAIN_VIEW] = &p;
		_views[SUB_VIEW] = &s;
	}

	bool operator()(const TaskLstFnStatus &l, const TaskLstFnStatus &r) const {
		return _views[l._iView]->getBufferAt(l._docIndex).getRecentTag() > _views[r._iView]->getBufferAt(r._docIndex).getRecentTag();
	}
};

Notepad_plus::Notepad_plus(): Window(), _mainWindowStatus(0), _pDocTab(NULL), _pEditView(NULL),
	_pMainSplitter(NULL), _isfullScreen(false),
    _recordingMacro(false), _pTrayIco(NULL), _isUDDocked(false), _isRTL(false),
	_linkTriggered(true), _isDocModifing(false), _isHotspotDblClicked(false), _isSaving(false), _sysMenuEntering(false)
{
	ZeroMemory(&_prevSelectedRange, sizeof(_prevSelectedRange));

    _winVersion = getWindowsVersion();

	TiXmlDocument *nativeLangDocRoot = (NppParameters::getInstance())->getNativeLang();
	if (nativeLangDocRoot)
	{
		_nativeLang =  nativeLangDocRoot->FirstChild("NotepadPlus");
		if (_nativeLang)
		{
			_nativeLang = _nativeLang->FirstChild("Native-Langue");
			if (_nativeLang)
			{
				TiXmlElement *element = _nativeLang->ToElement();
				const char *rtl = element->Attribute("RTL");
				if (rtl)
					_isRTL = (strcmp(rtl, "yes") == 0);
			}	
		}
	}
	else
		_nativeLang = NULL;
	
	TiXmlDocument *toolIconsDocRoot = (NppParameters::getInstance())->getToolIcons();
	if (toolIconsDocRoot)
	{
		_toolIcons =  toolIconsDocRoot->FirstChild("NotepadPlus");
		if (_toolIcons)
		{
			if ((_toolIcons = _toolIcons->FirstChild("ToolBarIcons")))
			{
				if ((_toolIcons = _toolIcons->FirstChild("Theme")))
				{
					const char *themeDir = (_toolIcons->ToElement())->Attribute("pathPrefix");

					for (TiXmlNode *childNode = _toolIcons->FirstChildElement("Icon");
						 childNode ;
						 childNode = childNode->NextSibling("Icon") )
					{
						int iIcon;
						const char *res = (childNode->ToElement())->Attribute("id", &iIcon);
						if (res)
						{
							TiXmlNode *grandChildNode = childNode->FirstChildElement("normal");
							if (grandChildNode)
							{
								TiXmlNode *valueNode = grandChildNode->FirstChild();
								//putain, enfin!!!
								if (valueNode)
								{
									string locator = themeDir?themeDir:"";
									
									locator += valueNode->Value();
									_customIconVect.push_back(iconLocator(0, iIcon, locator));
								}
							}

							grandChildNode = childNode->FirstChildElement("hover");
							if (grandChildNode)
							{
								TiXmlNode *valueNode = grandChildNode->FirstChild();
								//putain, enfin!!!
								if (valueNode)
								{
									string locator = themeDir?themeDir:"";
									
									locator += valueNode->Value();
									_customIconVect.push_back(iconLocator(1, iIcon, locator));
								}
							}

							grandChildNode = childNode->FirstChildElement("disabled");
							if (grandChildNode)
							{
								TiXmlNode *valueNode = grandChildNode->FirstChild();
								//putain, enfin!!!
								if (valueNode)
								{
									string locator = themeDir?themeDir:"";
									
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
	if (_pTrayIco)
		delete _pTrayIco;
}

void Notepad_plus::init(HINSTANCE hInst, HWND parent, const char *cmdLine, CmdLineParams *cmdLineParams)
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
		systemMessage("System Err");
		throw int(98);
	}

	RECT workAreaRect;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

	const NppGUI & nppGUI = (NppParameters::getInstance())->getNppGUI();

	if (cmdLineParams->_isNoPlugin)
		_pluginsManager.disable();

	_hSelf = ::CreateWindowEx(
					WS_EX_ACCEPTFILES | (_isRTL?WS_EX_LAYOUTRTL:0),\
					_className,\
					"Notepad++",\
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
		systemMessage("System Err");
		throw int(777);
	}

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

	if (nppGUI._rememberLastSession && !cmdLineParams->_isNoSession)
	{
		loadLastSession();
	}

    if (cmdLine)
    {
		char currPath[MAX_PATH];
		::GetCurrentDirectory(sizeof currPath, currPath);
		::SetCurrentDirectory(currPath);

		LangType lt = cmdLineParams->_langType;
		int ln = cmdLineParams->_line2go; 

		if (PathFileExists(cmdLine))
		{
			doOpen(cmdLine, cmdLineParams->_isReadOnly);
				
			if (lt != L_TXT)
				_pEditView->setCurrentDocType(lt);
			if (ln > 0)
                _pEditView->execute(SCI_GOTOLINE, ln-1);
		}
		else
		{
			FileNameStringSplitter fnss(cmdLine);
			char *pFn = NULL;
			
			for (int i = 0 ; i < fnss.size() ; i++)
			{
				pFn = (char *)fnss.getFileName(i);
				doOpen((const char *)pFn, cmdLineParams->_isReadOnly);
				
				if (lt != L_TXT)
					_pEditView->setCurrentDocType(lt);
				if (ln > 0)
					_pEditView->execute(SCI_GOTOLINE, ln-1);
			}
		}
		// restore the doc type to L_TXT
		//(NppParameters::getInstance())->setDefLang(L_TXT);
		
    }

	::GetModuleFileName(NULL, _nppPath, MAX_PATH);

	setTitleWith(_pEditView->getCurrentTitle());

	if (nppGUI._tabStatus & TAB_MULTILINE)
		::SendMessage(_hSelf, WM_COMMAND, IDM_VIEW_DRAWTABBAR_MULTILINE, 0);

	// Notify plugins that Notepad++ is ready
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_READY;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = 0;
	_pluginsManager.notify(&scnN);

	if (!nppGUI._menuBarShow)
		::SetMenu(_hSelf, NULL);

	::ShowWindow(_hSelf, nppGUI._isMaximized?SW_MAXIMIZE:SW_SHOW);
	if (cmdLineParams->_isNoTab || (nppGUI._tabStatus & TAB_HIDE))
	{
		::SendMessage(_hSelf, NPPM_HIDETABBAR, 0, TRUE);
	}
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
	unsigned char floatContArray[50];
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
	size_t i = 0;
	for ( ; i < session.nbMainFiles() ; )
	{
		const char *pFn = session._mainViewFiles[i]._fileName.c_str();
		if (PathFileExists(pFn))
		{
			doOpen(pFn);
			const char *pLn = session._mainViewFiles[i]._langName.c_str();
			setLangFromName(pLn);

			_pEditView->getCurrentBuffer().setPosition(session._mainViewFiles[i]);
			_pEditView->restoreCurrentPos();

			for (size_t j = 0 ; j < session._mainViewFiles[i].marks.size() ; j++)
				bookmarkAdd(session._mainViewFiles[i].marks[j]);

			i++;
		}
		else
		{
			vector<sessionFileInfo>::iterator posIt = session._mainViewFiles.begin() + i;
			session._mainViewFiles.erase(posIt);
			allSessionFilesLoaded = false;
		}
	}

	bool isSubViewOpened = false;
	for (size_t k = 0 ; k < session.nbSubFiles() ; k++)
	{
		const char *pFn = session._subViewFiles[k]._fileName.c_str();
		if (PathFileExists(pFn))
		{
			if (doOpen(pFn))
			{
				if (!isSubViewOpened)
				{
					docGotoAnotherEditView(MODE_TRANSFER); 
					isSubViewOpened = true;
				}
			}
			else // switch back to Main view where this file is already opened
			{
				docGotoAnotherEditView(MODE_CLONE);
				isSubViewOpened = true;
			}
			const char *pLn = session._subViewFiles[k]._langName.c_str();
			setLangFromName(pLn);

			_pEditView->getCurrentBuffer().setPosition(session._subViewFiles[k]);
			_pEditView->restoreCurrentPos();

			for (size_t j = 0 ; j < session._subViewFiles[k].marks.size() ; j++)
				bookmarkAdd(session._subViewFiles[k].marks[j]);
		}
		else
		{
			vector<sessionFileInfo>::iterator posIt = session._subViewFiles.begin() + k;
			session._subViewFiles.erase(posIt);
			allSessionFilesLoaded = false;
		}
	}
	if (session._activeMainIndex < (size_t)_mainDocTab.nbItem())//session.nbMainFiles())
		_mainDocTab.activate(session._activeMainIndex);

	if (session._activeSubIndex < (size_t)_subDocTab.nbItem())//session.nbSubFiles())
		_subDocTab.activate(session._activeSubIndex);

	if ((session.nbSubFiles() > 0) && (session._activeView == MAIN_VIEW || session._activeView == SUB_VIEW))
		switchEditViewTo(session._activeView);
	else
		switchEditViewTo(MAIN_VIEW);

	return allSessionFilesLoaded;
}

bool Notepad_plus::doSimpleOpen(const char *fileName)
{
	Utf8_16_Read UnicodeConvertor;

	FILE *fp = fopen(fileName, "rb");

	if (fp)
	{
		_pEditView->execute(SCI_CLEARALL, 0);
		_pEditView->setCurrentTitle(fileName);

		char data[blockSize];

		size_t lenFile = fread(data, 1, sizeof(data), fp);
		bool isNotEmpty = (lenFile != 0);

		while (lenFile > 0) 
		{
			lenFile = UnicodeConvertor.convert(data, lenFile);
			_pEditView->execute(SCI_ADDTEXT, lenFile, reinterpret_cast<LPARAM>(UnicodeConvertor.getNewBuf()));
			lenFile = int(fread(data, 1, sizeof(data), fp));
		}
		fclose(fp);
		
		UniMode unicodeMode = static_cast<UniMode>(UnicodeConvertor.getEncoding());
		(_pEditView->getCurrentBuffer()).setUnicodeMode(unicodeMode);

		if (unicodeMode != uni8Bit)
			// Override the code page if Unicode
			_pEditView->execute(SCI_SETCODEPAGE, SC_CP_UTF8);

		// Then replace the caret to the begining
		_pEditView->execute(SCI_GOTOPOS, 0);
		return true;
	}
	else
	{
		char msg[MAX_PATH + 100];
		strcpy(msg, "Can not open file \"");
		strcat(msg, fileName);
		strcat(msg, "\".");
		::MessageBox(_hSelf, msg, "Open File error", MB_OK);
		return false;
	}
}


bool Notepad_plus::doOpen(const char *fileName, bool isReadOnly)
{	
	char longFileName[MAX_PATH];
	::GetFullPathName(fileName, MAX_PATH, longFileName, NULL);

	if (switchToFile(longFileName))
	{
		if (_pTrayIco)
		{
			if (_pTrayIco->isInTray())
			{
				::ShowWindow(_hSelf, SW_SHOW);
				_pTrayIco->doTrayIcon(REMOVE);
				::SendMessage(_hSelf, WM_SIZE, 0, 0);
			}
		}
		return false;
	}
	
	if (!PathFileExists(longFileName))
	{
		char str2display[MAX_PATH*2];
		char longFileDir[MAX_PATH];

		strcpy(longFileDir, longFileName);
		PathRemoveFileSpec(longFileDir);

		if (PathFileExists(longFileDir))
		{
			sprintf(str2display, "%s doesn't exist. Create it?", longFileName);

			if (::MessageBox(_hSelf, str2display, "Create new file", MB_YESNO) == IDYES)
			{
				FILE *f = fopen(longFileName, "w");
				fclose(f);
			}
			else
			{
				_lastRecentFileList.remove(longFileName);
				return false;
			}
		}
		else
		{
			_lastRecentFileList.remove(longFileName);
			return false;
		}
	}

	// if file2open matches the ext of user defined session file ext, then it'll be opened as a session
	const char *definedSessionExt = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();
	if (*definedSessionExt != '\0')
	{
		char fncp[MAX_PATH];
		strcpy(fncp, longFileName);
		char *pExt = PathFindExtension(fncp);
		string usrSessionExt = "";
		if (*definedSessionExt != '.')
		{
			usrSessionExt += ".";
		}
		usrSessionExt += definedSessionExt;

		if (!strcmp(pExt, usrSessionExt.c_str()))
		{
			return fileLoadSession(longFileName);
		}
	}
	Utf8_16_Read UnicodeConvertor;

    bool isNewDoc2Close = false;
	FILE *fp = fopen(longFileName, "rb");
   
	if (fp)
	{
		// Notify plugins that current file is just opened
		SCNotification scnN;
		scnN.nmhdr.code = NPPN_FILEBEFOREOPEN;
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = 0;
		_pluginsManager.notify(&scnN);

        if ((_pEditView->getNbDoc() == 1) 
			&& Buffer::isUntitled(_pEditView->getCurrentTitle())
            && (!_pEditView->isCurrentDocDirty()) && (_pEditView->getCurrentDocLen() == 0))
        {
            isNewDoc2Close = true;
        }
		setTitleWith(_pDocTab->newDoc(longFileName));

		// It's VERY IMPORTANT to reset the view
		_pEditView->execute(SCI_CLEARALL);

		char data[blockSize];
		size_t lenFile = fread(data, 1, sizeof(data), fp);
		bool isNotEmpty = (lenFile != 0);

		while (lenFile > 0) 
		{
			lenFile = UnicodeConvertor.convert(data, lenFile);
			_pEditView->execute(SCI_ADDTEXT, lenFile, reinterpret_cast<LPARAM>(UnicodeConvertor.getNewBuf()));
			lenFile = int(fread(data, 1, sizeof(data), fp));
		}
		fclose(fp);

		// 3 formats : WIN_FORMAT, UNIX_FORMAT and MAC_FORMAT
		(_pEditView->getCurrentBuffer()).determinateFormat(isNotEmpty?UnicodeConvertor.getNewBuf():(char *)(""));
		_pEditView->execute(SCI_SETEOLMODE, _pEditView->getCurrentBuffer().getFormat());

		// detect if it's a binary file (non ascii file) 
		//(_pEditView->getCurrentBuffer()).detectBin(isNotEmpty?UnicodeConvertor.getNewBuf():(char *)(""));

		UniMode unicodeMode = static_cast<UniMode>(UnicodeConvertor.getEncoding());
		(_pEditView->getCurrentBuffer()).setUnicodeMode(unicodeMode);

		if (unicodeMode != uni8Bit)
			// Override the code page if Unicode
			_pEditView->execute(SCI_SETCODEPAGE, SC_CP_UTF8);

		if (isReadOnly)
			(_pEditView->getCurrentBuffer()).setReadOnly(true);

		_pEditView->getFocus();
		_pEditView->execute(SCI_SETSAVEPOINT);
		_pEditView->execute(EM_EMPTYUNDOBUFFER);

		// if file is read only, we set the view read only
		_pEditView->execute(SCI_SETREADONLY, _pEditView->isCurrentBufReadOnly());
        if (isNewDoc2Close)
            _pDocTab->closeDocAt(0);

		int numLines = int(_pEditView->execute(SCI_GETLINECOUNT));

		char numLineStr[32];
		itoa(numLines, numLineStr, 10);
		int nbDigit = strlen(numLineStr);

		if (_pEditView->increaseMaxNbDigit(nbDigit))
			_pEditView->setLineNumberWidth(_pEditView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER));

		// Then replace the caret to the begining
		_pEditView->execute(SCI_GOTOPOS, 0);
		_lastRecentFileList.remove(longFileName);
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
		setWorkingDir(longFileName);

		// Notify plugins that current file is just opened
		scnN.nmhdr.code = NPPN_FILEOPENED;
		_pluginsManager.notify(&scnN);

		return true;
	}
	else
	{
		char msg[MAX_PATH + 100];
		strcpy(msg, "Can not open file \"");
		//strcat(msg, fullPath);
		strcat(msg, longFileName);
		strcat(msg, "\".");
		::MessageBox(_hSelf, msg, "ERR", MB_OK);
		_lastRecentFileList.remove(longFileName);
		return false;
	}
}
void Notepad_plus::fileNew()
{
	setTitleWith(_pDocTab->newDoc(NULL));
	setWorkingDir(NULL);
}

bool Notepad_plus::fileReload()
{
	const char * fn = _pEditView->getCurrentTitle();
	if (Buffer::isUntitled(fn)) return false;
	if (::MessageBox(_hSelf, "Do you want to reload the current file?", "Reload", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
		reload(fn);
	return true;
}
string exts2Filters(string exts) {
	const char *extStr = exts.c_str();
	char aExt[MAX_PATH];
	string filters("");

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
					filters += "*.";
					filters += aExt;
					filters += ";";
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
			filters += "*.";
			filters += aExt;
			filters += ";";
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
	LangType curl = _pEditView->getCurrentBuffer().getLangType();

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
			const char *defList = l->getDefaultExtList();
			const char *userList = NULL;

			LexerStylerArray &lsa = (NppParameters::getInstance())->getLStylerArray();
			const char *lName = l->getLangName();
			LexerStyler *pLS = lsa.getLexerStylerByName(lName);
			
			if (pLS)
				userList = pLS->getLexerUserExt();

			std::string list("");
			if (defList)
				list += defList;
			if (userList)
			{
				list += " ";
				list += userList;
			}
			
			string stringFilters = exts2Filters(list);
			const char *filters = stringFilters.c_str();
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
	fDlg.setExtFilter("All types", ".*", NULL);
	
	setFileOpenSaveDlgFilters(fDlg);

	if (stringVector *pfns = fDlg.doOpenMultiFilesDlg())
	{
		int sz = int(pfns->size());
		for (int i = 0 ; i < sz ; i++)
			doOpen((pfns->at(i)).c_str(), fDlg.isReadOnly());
	}
}



bool Notepad_plus::doReload(const char *fileName, bool alert)
{
	char longFileName[MAX_PATH] ="";
	::GetFullPathName(fileName, MAX_PATH, longFileName, NULL);

	if (switchToFile(longFileName))
	{
		if (alert)
		{
			if (::MessageBox(_hSelf, "Do you want to reload the current file?", "Reload", MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
				reload(longFileName);
		}
		else
			reload(longFileName);

		return true;
	}
	return false;
}

bool Notepad_plus::doSave(const char *filename, UniMode mode)
{
	bool isHidden = false;
	bool isSys = false;
	DWORD attrib;

	if (PathFileExists(filename))
	{
		attrib = ::GetFileAttributes(filename);

		if (attrib != INVALID_FILE_ATTRIBUTES)
		{
			isHidden = (attrib & FILE_ATTRIBUTE_HIDDEN) != 0;
			if (isHidden)
				::SetFileAttributes(filename, attrib & ~FILE_ATTRIBUTE_HIDDEN);

			isSys = (attrib & FILE_ATTRIBUTE_SYSTEM) != 0;
			if (isSys)
				::SetFileAttributes(filename, attrib & ~FILE_ATTRIBUTE_SYSTEM);
			
		}
	}

	if (mode == uniCookie)
		mode = uni8Bit;

	Utf8_16_Write UnicodeConvertor;
	if (_pEditView->execute(SCI_GETCODEPAGE) != 0)
		UnicodeConvertor.setEncoding(static_cast<UniMode>(mode));

	FILE *fp = UnicodeConvertor.fopen(filename, "wb");
	
	if (fp)
	{
		// Notify plugins that current file is about to be saved
		SCNotification scnN;
		scnN.nmhdr.code = NPPN_FILEBEFORESAVE;
		scnN.nmhdr.hwndFrom = _hSelf;
		scnN.nmhdr.idFrom = 0;
		_pluginsManager.notify(&scnN);

		char data[blockSize + 1];
		int lengthDoc = _pEditView->getCurrentDocLen();
		for (int i = 0; i < lengthDoc; i += blockSize)
		{
			int grabSize = lengthDoc - i;
			if (grabSize > blockSize) 
				grabSize = blockSize;
			
			_pEditView->getText(data, i, i + grabSize);
			UnicodeConvertor.fwrite(data, grabSize);
		}
		UnicodeConvertor.fclose();

		_pEditView->updateCurrentBufTimeStamp();
		_pEditView->execute(SCI_SETSAVEPOINT);

		if (isHidden)
			::SetFileAttributes(filename, attrib | FILE_ATTRIBUTE_HIDDEN);

		if (isSys)
			::SetFileAttributes(filename, attrib | FILE_ATTRIBUTE_SYSTEM);

		scnN.nmhdr.code = NPPN_FILESAVED;
		_pluginsManager.notify(&scnN);
		return true;
	}
	::MessageBox(_hSelf, "Please check whether if this file is opened in another program", "Save failed", MB_OK);
	return false;
}

bool Notepad_plus::fileSave()
{
	if (_pEditView->isCurrentDocDirty())
	{
		const char *fn = _pEditView->getCurrentTitle();
		if (Buffer::isUntitled(fn))
		{
			return fileSaveAs();
		}
		else
		{
			const NppGUI & nppgui = (NppParameters::getInstance())->getNppGUI();
			BackupFeature backup = nppgui._backup;
			if (backup == bak_simple)
			{
				//copy fn to fn.backup
				string fn_bak(fn);
				if ((nppgui._useDir) && (nppgui._backupDir[0] != '\0'))
				{
					char path[MAX_PATH];
					char *name;

					strcpy(path, fn);
					name = ::PathFindFileName(path);
					fn_bak = nppgui._backupDir;
					fn_bak += "\\";
					fn_bak += name;
				}
				else
				{
					fn_bak = fn;
				}
				fn_bak += ".bak";
				::CopyFile(fn, fn_bak.c_str(), FALSE);
			}
			else if (backup == bak_verbose)
			{
				char path[MAX_PATH];
				char *name;
				string fn_dateTime_bak;
				
				strcpy(path, fn);

				name = ::PathFindFileName(path);
				::PathRemoveFileSpec(path);
				
				if ((nppgui._useDir) && (nppgui._backupDir[0] != '\0'))
				{//printStr(nppgui._backupDir);
					fn_dateTime_bak = nppgui._backupDir;
					fn_dateTime_bak += "\\";
				}
				else
				{
					const char *bakDir = "nppBackup";
					fn_dateTime_bak = path;
					fn_dateTime_bak += "\\";
					fn_dateTime_bak += bakDir;
					fn_dateTime_bak += "\\";

					if (!::PathFileExists(fn_dateTime_bak.c_str()))
					{
						::CreateDirectory(bakDir, NULL);
					}
				}

				fn_dateTime_bak += name;

				const int temBufLen = 32;
				char tmpbuf[temBufLen];
				time_t ltime = time(0);
				struct tm *today;

				today = localtime(&ltime);
				strftime(tmpbuf, temBufLen, "%Y-%m-%d_%H%M%S", today);

				fn_dateTime_bak += ".";
				fn_dateTime_bak += tmpbuf;
				fn_dateTime_bak += ".bak";

				::CopyFile(fn, fn_dateTime_bak.c_str(), FALSE);
			}
			return doSave(fn, _pEditView->getCurrentBuffer().getUnicodeMode());
		}
	}
	return false;
}

bool Notepad_plus::fileSaveAll() {

	int iCurrent = _pEditView->getCurrentDocIndex();

    if (_mainWindowStatus & TWO_VIEWS_MASK)
    {
        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
        int iCur = _pEditView->getCurrentDocIndex();

	    for (size_t i = 0 ; i < _pEditView->getNbDoc() ; i++)
	    {
		    _pDocTab->activate(i);
			if (!_pEditView->getCurrentBuffer().isReadOnly())
				fileSave();
	    }

        _pDocTab->activate(iCur);

        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
    }
    
    for (size_t i = 0 ; i < _pEditView->getNbDoc() ; i++)
	{
		_pDocTab->activate(i);
		if (!_pEditView->getCurrentBuffer().isReadOnly())
			fileSave();
	}

	_pDocTab->activate(iCurrent);
	return true;
}

bool Notepad_plus::replaceAllFiles() {

	int iCurrent = _pEditView->getCurrentDocIndex();
	int nbTotal = 0;
	const bool isEntireDoc = true;

    if (_mainWindowStatus & TWO_VIEWS_MASK)
    {
        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
        int iCur = _pEditView->getCurrentDocIndex();

		for (size_t i = 0 ; i < _pEditView->getNbDoc() ; i++)
	    {
		    _pDocTab->activate(i);
			if (!_pEditView->getCurrentBuffer().isReadOnly())
			{
				_pEditView->execute(SCI_BEGINUNDOACTION);
				nbTotal += _findReplaceDlg.processAll(REPLACE_ALL, isEntireDoc);
				_pEditView->execute(SCI_ENDUNDOACTION);
			}
	    }
        _pDocTab->activate(iCur);
        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
    }
    
    for (size_t i = 0 ; i < _pEditView->getNbDoc() ; i++)
	{
		_pDocTab->activate(i);
		if (!_pEditView->getCurrentBuffer().isReadOnly())
		{
			_pEditView->execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(REPLACE_ALL, isEntireDoc);
			_pEditView->execute(SCI_ENDUNDOACTION);
		}
	}

	_pDocTab->activate(iCurrent);

	char result[64];
	if (nbTotal < 0)
		strcpy(result, "The regular expression to search is formed badly");
	else
	{
		itoa(nbTotal, result, 10);
		strcat(result, " tokens are replaced.");
		
	}
	::MessageBox(_hSelf, result, "", MB_OK);

	return true;
}

bool Notepad_plus::matchInList(const char *fileName, const vector<string> & patterns)
{
	for (size_t i = 0 ; i < patterns.size() ; i++)
	{
		if (PathMatchSpec(fileName, patterns[i].c_str()))
			return true;
	}
	return false;
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
		{
			_pEditView->execute(SCI_SETTARGETSTART, i + 1);
			_pEditView->execute(SCI_SETTARGETEND, lineEnd);
			_pEditView->execute(SCI_REPLACETARGET, 0, (LPARAM)"");
		}
	}
	_pEditView->execute(SCI_ENDUNDOACTION);
}

void Notepad_plus::loadLastSession() 
{
	Session lastSession = (NppParameters::getInstance())->getSession();
	loadSession(lastSession);
}

void Notepad_plus::getMatchedFileNames(const char *dir, const vector<string> & patterns, vector<string> & fileNames, bool isRecursive)
{
	string dirFilter(dir);
	dirFilter += "*.*";
	WIN32_FIND_DATA foundData;

	HANDLE hFile = ::FindFirstFile(dirFilter.c_str(), &foundData);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		
		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{
				// branles rien
			}
			else if (isRecursive)
			{
				if ((strcmp(foundData.cFileName, ".")) && (strcmp(foundData.cFileName, "..")))
				{
					string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += "\\";
					getMatchedFileNames(pathDir.c_str(), patterns, fileNames, isRecursive);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				string pathFile(dir);
				pathFile += foundData.cFileName;
				fileNames.push_back(pathFile.c_str());
			}
		}
	}
	while (::FindNextFile(hFile, &foundData))
	{
		if (foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (foundData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{
				// branles rien
			}
			else if (isRecursive)
			{
				if ((strcmp(foundData.cFileName, ".")) && (strcmp(foundData.cFileName, "..")))
				{
					string pathDir(dir);
					pathDir += foundData.cFileName;
					pathDir += "\\";
					getMatchedFileNames(pathDir.c_str(), patterns, fileNames, isRecursive);
				}
			}
		}
		else
		{
			if (matchInList(foundData.cFileName, patterns))
			{
				string pathFile(dir);
				pathFile += foundData.cFileName;
				fileNames.push_back(pathFile.c_str());
			}
		}
	}
	::FindClose(hFile);
}

bool Notepad_plus::findInFiles(bool isRecursive)
{
	int nbTotal = 0;
	ScintillaEditView *pOldView = _pEditView;

	_pEditView = &_invisibleEditView;
	_findReplaceDlg.setFinderReadOnly(false);

	if (!_findReplaceDlg.isFinderEmpty())
		_findReplaceDlg.clearFinder();

	const char *dir2Search = _findReplaceDlg.getDir2Search();

	if (!dir2Search[0] || !::PathFileExists(dir2Search))
	{
		return false;
	}
	vector<string> patterns2Match;
	if (_findReplaceDlg.getFilters() == "")
		_findReplaceDlg.setFindInFilesDirFilter(NULL, "*.*");
	_findReplaceDlg.getPatterns(patterns2Match);
	vector<string> fileNames;
	getMatchedFileNames(dir2Search, patterns2Match, fileNames, isRecursive);

	for (size_t i = 0 ; i < fileNames.size() ; i++)
	{
		const char *fn = fileNames[i].c_str();
		if (doSimpleOpen(fn))
			nbTotal += _findReplaceDlg.processAll(FIND_ALL, true, fn);
	}
	_findReplaceDlg.setFinderReadOnly();
	_findReplaceDlg.putFindResult(nbTotal);

	_pEditView = pOldView;
	return true;
}

bool Notepad_plus::findInOpenedFiles() {

	int iCurrent = _pEditView->getCurrentDocIndex();
	int nbTotal = 0;
	const bool isEntireDoc = true;

	_findReplaceDlg.setFinderReadOnly(false);
	//_findReplaceDlg.setFinderStyle();
	//_pFinder->defineDocType(L_TXT);
	//_pFinder->execute(SCI_STYLESETSIZE, STYLE_DEFAULT, 8);

	if (!_findReplaceDlg.isFinderEmpty())
		_findReplaceDlg.clearFinder();
	
	_findReplaceDlg.setSearchWord2Finder();

    if (_mainWindowStatus & TWO_VIEWS_MASK)
    {
        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
        int iCur = _pEditView->getCurrentDocIndex();
		
	    for (size_t i = 0 ; i < _pEditView->getNbDoc() ; i++)
	    {
		    _pDocTab->activate(i);

			_pEditView->execute(SCI_BEGINUNDOACTION);
			nbTotal += _findReplaceDlg.processAll(FIND_ALL, isEntireDoc, _pEditView->getCurrentTitle());
			_pEditView->execute(SCI_ENDUNDOACTION);
			
	    }
        _pDocTab->activate(iCur);
        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
    }
    
    for (size_t i = 0 ; i < _pEditView->getNbDoc() ; i++)
	{
		_pDocTab->activate(i);
		
		_pEditView->execute(SCI_BEGINUNDOACTION);
		nbTotal += _findReplaceDlg.processAll(FIND_ALL, isEntireDoc, _pEditView->getCurrentTitle());
		_pEditView->execute(SCI_ENDUNDOACTION);
	}

	_pDocTab->activate(iCurrent);
	
	_findReplaceDlg.setFinderReadOnly();

	_findReplaceDlg.putFindResult(nbTotal);
	
	return true;
}

bool Notepad_plus::fileSaveAs()
{
	FileDialog fDlg(_hSelf, _hInst);

    fDlg.setExtFilter("All types", ".*", NULL);
	setFileOpenSaveDlgFilters(fDlg);

	char str[MAX_PATH];
	strcpy(str, _pEditView->getCurrentTitle());
			
	fDlg.setDefFileName(PathFindFileName(str));

	int currentDocIndex = _pEditView->getCurrentDocIndex();
	_isSaving = true;
	char *pfn = fDlg.doSaveDlg();
	_isSaving = false;
	if (pfn)
	{
		int i = _pEditView->findDocIndexByName(pfn);
		if ((i == -1) || (i == currentDocIndex))
		{
			doSave(pfn, _pEditView->getCurrentBuffer().getUnicodeMode());
			_pEditView->setCurrentTitle(pfn);
            _pEditView->setCurrentDocReadOnly(false);
			_pDocTab->updateCurrentTabItem(PathFindFileName(pfn));
			setTitleWith(pfn);
			setLangStatus(_pEditView->getCurrentDocType());
			checkLangsMenu(-1);
			return true;
		}
		else
		{
			::MessageBox(_hSelf, "The file is already opened in the Notepad++.", "ERROR", MB_OK | MB_ICONSTOP);
			_pDocTab->activate(i);
			return false;
		}
        checkModifiedDocument();
	}
	else // cancel button is pressed
    {
        checkModifiedDocument();
		return false;
    }
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
	bool isCurrentDirty = _pEditView->isCurrentDocDirty();
	bool isSeveralDirty = (!_pEditView->isAllDocsClean()) || (!getNonCurrentEditView()->isAllDocsClean());

	enableCommand(IDM_FILE_SAVE, isCurrentDirty, MENU | TOOLBAR);
	enableCommand(IDM_FILE_SAVEALL, isSeveralDirty, MENU | TOOLBAR);
	
	bool isSysReadOnly = _pEditView->isCurrentBufSysReadOnly();
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
		bool isUserReadOnly = _pEditView->isCurrentBufUserReadOnly();
		::CheckMenuItem(_mainMenuHandle, IDM_EDIT_SETREADONLY, MF_BYCOMMAND | (isUserReadOnly?MF_CHECKED:MF_UNCHECKED));
	}

	enableConvertMenuItems((_pEditView->getCurrentBuffer()).getFormat());
	checkUnicodeMenuItems((_pEditView->getCurrentBuffer()).getUnicodeMode());
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
	bool canDoSync = _mainDocTab.isVisible() && _subDocTab.isVisible();
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

void Notepad_plus::synchronise()
{
    Buffer & bufSrc = _pEditView->getCurrentBuffer();
    
    const char *fn = bufSrc.getFileName();

    int i = getNonCurrentDocTab()->find(fn);
    if (i != -1)
    {
        Buffer & bufDest = getNonCurrentEditView()->getBufferAt(i);
        bufDest.synchroniseWith(bufSrc);
        getNonCurrentDocTab()->updateTabItem(i);
    }
}


void Notepad_plus::checkLangsMenu(int id) const 
{
	if (id == -1)
	{
		id = (NppParameters::getInstance())->langTypeToCommandID(_pEditView->getCurrentDocType());
		if (id == IDM_LANG_USER)
		{
			if (_pEditView->getCurrentBuffer().isUserDefineLangExt())
			{
				const char *userLangName = _pEditView->getCurrentBuffer().getUserDefineLangName();
				char menuLangName[16];

				for (int i = IDM_LANG_USER + 1 ; i <= IDM_LANG_USER_LIMIT ; i++)
				{
					if (::GetMenuString(_mainMenuHandle, i, menuLangName, sizeof(menuLangName), MF_BYCOMMAND))
						if (!strcmp(userLangName, menuLangName))
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

string Notepad_plus::getLangDesc(LangType langType, bool shortDesc)
{

	if ((langType >= L_EXTERNAL) && (langType < NppParameters::getInstance()->L_END))
	{
		ExternalLangContainer & elc = NppParameters::getInstance()->getELCFromIndex(langType - L_EXTERNAL);
		if (shortDesc)
			return string(elc._name);
		else
			return string(elc._desc);
	}

	static const int maxChar = 64;
	static char langDescArray[][maxChar] = {
		"Normal text",				"Normal text file",
		"PHP",						"PHP Hypertext Preprocessor file",
		"C",						"C source file",
		"C++",						"C++ source file",
		"C#",						"C# source file",
		"Objective-C",				"Objective-C source file",
		"Java",						"Java source file",
		"RC",						"Windows Resource file",
		"HTML",						"Hyper Text Markup Language file",
		"XML",						"eXtensible Markup Language file",
		"Makefile",					"Makefile",
		"Pascal",					"Pascal source file",
		"Batch",					"Batch file",
		"ini",						"MS ini file",
		"NFO",						"MSDOS Style/ASCII Art",
		"udf",						"User Define File",
		"ASP",						"Active Server Pages script file",
		"SQL",						"Structured Query Language file",
		"VB",						"Visual Basic file",
		"JavaScript",				"JavaScript file",
		"CSS",						"Cascade Style Sheets File",
		"Perl",						"Perl source file",
		"Python",					"Python file",
		"Lua",						"Lua source File",
		"TeX",						"TeX file",
		"Fortran",					"Fortran source file",
		"Shell",					"Unix script file",
		"Flash Action",				"Flash Action script file",
		"NSIS",						"Nullsoft Scriptable Install System script file",
		"TCL",						"Tool Command Language file",
		"Lisp",						"List Processing language file",
		"Scheme",					"Scheme file",
		"Assembly",					"Assembly language source file",
		"Diff",						"Diff file",
		"Properties file",			"Properties file",
		"Postscript",				"Postscript file",
		"Ruby",						"Ruby file",
		"Smalltalk",				"Smalltalk file",
		"VHDL",						"VHSIC Hardware Description Language file",
		"KiXtart",					"KiXtart file",
		"AutoIt",					"AutoIt",
		"CAML",						"Categorical Abstract Machine Language",
		"Ada",						"Ada file",
		"Verilog",					"Verilog file",
		"MATLAB",					"MATrix LABoratory",
		"Haskell",					"Haskell",
		"Inno",						"Inno Setup script",
		"Internal Search",			"Internal Search",
		"CMAKEFILE",				"CMAKEFILE",
		"YAML",				"YAML Ain't Markup Language"
	};

	int index = (int(langType)) * 2 + (shortDesc?0:1);
	if (index >= sizeof(langDescArray)/maxChar)
		index = 0;

	string str2Show = langDescArray[index];

	if (langType == L_USER)
	{
		Buffer & currentBuf = _pEditView->getCurrentBuffer();
		if (currentBuf.isUserDefineLangExt())
		{
			str2Show += " - ";
			str2Show += currentBuf.getUserDefineLangName();
		}
	}
	return str2Show;
}

void Notepad_plus::getApiFileName(LangType langType, string &fn)
{

    switch (langType)
    {
	case L_C: fn = "c";	break;

	case L_CPP:	fn = "cpp";	break;

	case L_OBJC: fn = "objC"; break;

	case L_JAVA: fn = "java"; break;

    case L_CS : fn = "cs"; break;

    case L_XML: fn = "xml"; break;

	case L_JS: fn = "javascript"; break;

	case L_PHP: fn = "php"; break;

	case L_VB:
	case L_ASP: fn = "vb"; break;

    case L_CSS: fn = "css"; break;

    case L_LUA: fn = "lua"; break;

    case L_PERL: fn = "perl"; break;

    case L_PASCAL: fn = "pascal"; break;

    case L_PYTHON: fn = "python"; break;

	case L_TEX : fn = "tex"; break;

	case L_FORTRAN : fn = "fortran"; break;

	case L_BASH : fn = "bash"; break;

	case L_FLASH :  fn = "flash"; break;

	case L_NSIS :  fn = "nsis"; break;

	case L_TCL :  fn = "tcl"; break;

	case L_LISP : fn = "lisp"; break;

	case L_SCHEME : fn = "scheme"; break;

	case L_ASM :
        fn = "asm"; break;

	case L_DIFF :
        fn = "diff"; break;
/*
	case L_PROPS :
        fn = "Properties file"; break;
*/
	case L_PS :
        fn = "postscript"; break;

	case L_RUBY :
        fn = "ruby"; break;

	case L_SMALLTALK :
        fn = "smalltalk"; break;

	case L_VHDL :
        fn = "vhdl"; break;

	case L_KIX :
        fn = "kix"; break;

	case L_AU3 :
        fn = "autoit"; break;

	case L_CAML :
        fn = "caml"; break;

	case L_ADA :
        fn = "ada"; break;

	case L_VERILOG :
        fn = "verilog"; break;

	case L_MATLAB :
        fn = "matlab"; break;

	case L_HASKELL :
        fn = "haskell"; break;

	case L_INNO :
        fn = "inno"; break;

	case L_CMAKE :
        fn = "cmake"; break;

	case L_YAML :
        fn = "yaml"; break;

	case L_USER :  
	{
		Buffer & currentBuf = _pEditView->getCurrentBuffer();
		if (currentBuf.isUserDefineLangExt())
		{
			fn = currentBuf.getUserDefineLangName();
		}
		break;
	}
    default:
		if (langType >= L_EXTERNAL && langType < NppParameters::getInstance()->L_END)
			fn = NppParameters::getInstance()->getELCFromIndex(langType - L_EXTERNAL)._name;
		else
			fn = "text";
    }
}


BOOL Notepad_plus::notify(SCNotification *notification)
{
	//Important, keep track of which element generated the message
	bool isFromPrimary = (_mainEditView.getHSelf() == notification->nmhdr.hwndFrom);
	ScintillaEditView * notifyView = isFromPrimary?&_mainEditView:&_subEditView;
	DocTabView *notifyDocTab = isFromPrimary?&_mainDocTab:&_subDocTab;
	switch (notification->nmhdr.code) 
	{
  
		case SCN_MODIFIED:
		{
			if ((notification->modificationType & SC_MOD_DELETETEXT) || (notification->modificationType & SC_MOD_INSERTTEXT))
			{
				_linkTriggered = true;
				_isDocModifing = true;
				::InvalidateRect(notifyView->getHSelf(), NULL, TRUE);
			}
			if (notification->modificationType & SC_MOD_CHANGEFOLD)
			{
				notifyView->foldChanged(notification->line,
				        notification->foldLevelNow, notification->foldLevelPrev);
			}
		}
		break;

		case SCN_SAVEPOINTREACHED:
			notifyView->setCurrentDocState(false);
			notifyDocTab->updateCurrentTabItem();
			checkDocState();
			synchronise();
			break;

		case SCN_SAVEPOINTLEFT:
			notifyView->setCurrentDocState(true);
			notifyDocTab->updateCurrentTabItem();
			checkDocState();
			synchronise();
			break;

		case  SCN_MODIFYATTEMPTRO :
			// on fout rien
			break;

		case SCN_KEY:
			break;

	case TCN_TABDROPPEDOUTSIDE:
	case TCN_TABDROPPED:
	{
        TabBarPlus *sender = reinterpret_cast<TabBarPlus *>(notification->nmhdr.idFrom);
        int destIndex = sender->getTabDraggedIndex();
		int scrIndex  = sender->getSrcTabIndex();

		// if the dragNdrop tab is not the current view tab,
		// we have to set it to the current view tab
		if (notification->nmhdr.hwndFrom != _pDocTab->getHSelf())
			switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);

        _pEditView->sortBuffer(destIndex, scrIndex);
        _pEditView->setCurrentIndex(destIndex);

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
					char goToView[64] = "Go to another View";
					char cloneToView[64] = "Clone to another View";
					const char *pGoToView = goToView;
					const char *pCloneToView = cloneToView;

					if (_nativeLang)
					{
						TiXmlNode *tabBarMenu = _nativeLang->FirstChild("Menu");
						tabBarMenu = tabBarMenu->FirstChild("TabBar");
						if (tabBarMenu)
						{
							for (TiXmlNode *childNode = tabBarMenu->FirstChildElement("Item");
								childNode ;
								childNode = childNode->NextSibling("Item") )
							{
								TiXmlElement *element = childNode->ToElement();
								int ordre;
								element->Attribute("order", &ordre);
								if (ordre == 5)
									pGoToView = element->Attribute("name");
								else if (ordre == 6)
									pCloneToView = element->Attribute("name");
							}
						}
						if (!pGoToView || !pGoToView[0])
							pGoToView = goToView;
						if (!pCloneToView || !pCloneToView[0])
							pCloneToView = cloneToView;
					}
					vector<MenuItemUnit> itemUnitArray;
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, pGoToView));
					itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, pCloneToView));
					_tabPopupDropMenu.create(_hSelf, itemUnitArray);
				}
				_tabPopupDropMenu.display(p);
			}
			else if ((hWin == getNonCurrentDocTab()->getHSelf()) || 
				     (hWin == getNonCurrentEditView()->getHSelf())) // In the another view group
			{
				if (::GetKeyState(VK_LCONTROL) & 0x80000000)
					docGotoAnotherEditView(MODE_CLONE);
				else
					docGotoAnotherEditView(MODE_TRANSFER);
			}
			//else on fout rien!!! // It's non view group
        }
		break;
	}

	case TCN_TABDELETE:
	{
		if (notification->nmhdr.hwndFrom != _pDocTab->getHSelf())
			switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);

		fileClose();
		break;

	}

	case TCN_SELCHANGE:
	{
        char fullPath[MAX_PATH];

        if (notification->nmhdr.hwndFrom == _mainDocTab.getHSelf())
		{
			strcpy(fullPath, _mainDocTab.clickedUpdate());
            switchEditViewTo(MAIN_VIEW);
			
		}
		else if (notification->nmhdr.hwndFrom == _subDocTab.getHSelf())
		{
			strcpy(fullPath, _subDocTab.clickedUpdate());
            switchEditViewTo(SUB_VIEW);
		}

		PathRemoveFileSpec(fullPath);
		setWorkingDir(fullPath);
		//_pEditView->execute(SCI_SETLEXER, SCLEX_CONTAINER)
		_linkTriggered = true;
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
				_statusBar.setText((_pEditView->execute(SCI_GETOVERTYPE))?"OVR":"INS", STATUSBAR_TYPING_MODE);
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
        
		POINT p, clientPoint;
		::GetCursorPos(&p);
        clientPoint.x = p.x;
        clientPoint.y = p.y;

		if (!_tabPopupMenu.isCreated())
		{
			char close[32] = "Close me";
			char closeBut[32] = "Close all but me";
			char save[32] = "Save me";
			char saveAs[32] = "Save me As...";
			char print[32] = "Print me";
			char readOnly[32] = "Read only";
			char clearReadOnly[32] = "Clear read only flag";
			char goToView[32] = "Go to another View";
			char cloneToView[32] = "Clone to another View";
			char cilpFullPath[32] = "Full file path to Clipboard";
			char cilpFileName[32] = "File name to Clipboard";
			char cilpCurrentDir[32] = "Current dir path to Clipboard";


			const char *pClose = close;
			const char *pCloseBut = closeBut;
			const char *pSave = save;
			const char *pSaveAs = saveAs;
			const char *pPrint = print;
			const char *pReadOnly = readOnly;
			const char *pClearReadOnly = clearReadOnly;
			const char *pGoToView = goToView;
			const char *pCloneToView = cloneToView;
			const char *pCilpFullPath = cilpFullPath;
			const char *pCilpFileName = cilpFileName;
			const char *pCilpCurrentDir = cilpCurrentDir;
			if (_nativeLang)
			{
				TiXmlNode *tabBarMenu = _nativeLang->FirstChild("Menu");
				if (tabBarMenu) 
				{
					tabBarMenu = tabBarMenu->FirstChild("TabBar");
					if (tabBarMenu)
					{
						for (TiXmlNode *childNode = tabBarMenu->FirstChildElement("Item");
							childNode ;
							childNode = childNode->NextSibling("Item") )
						{
							TiXmlElement *element = childNode->ToElement();
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

							}
						}
					}	
				}
				if (!pClose || !pClose[0])
					pClose = close;
				if (!pCloseBut || !pCloseBut[0])
					pCloseBut = cloneToView;
				if (!pSave || !pSave[0])
					pSave = save;
				if (!pSaveAs || !pSaveAs[0])
					pSaveAs = saveAs;
				if (!pPrint || !pPrint[0])
					pPrint = print;
				if (!pGoToView || !pGoToView[0])
					pGoToView = goToView;
				if (!pCloneToView || !pCloneToView[0])
					pCloneToView = cloneToView;
			}
			vector<MenuItemUnit> itemUnitArray;
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSE, pClose));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_CLOSEALL_BUT_CURRENT, pCloseBut));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVE, pSave));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_SAVEAS, pSaveAs));
			itemUnitArray.push_back(MenuItemUnit(IDM_FILE_PRINT, pPrint));
			itemUnitArray.push_back(MenuItemUnit(0, NULL));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_SETREADONLY, pReadOnly));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CLEARREADONLY, pClearReadOnly));
			itemUnitArray.push_back(MenuItemUnit(0, NULL));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FULLPATHTOCLIP,	pCilpFullPath));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_FILENAMETOCLIP,   pCilpFileName));
			itemUnitArray.push_back(MenuItemUnit(IDM_EDIT_CURRENTDIRTOCLIP, pCilpCurrentDir));
			itemUnitArray.push_back(MenuItemUnit(0, NULL));
			itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_GOTO_ANOTHER_VIEW, pGoToView));
			itemUnitArray.push_back(MenuItemUnit(IDM_VIEW_CLONE_TO_ANOTHER_VIEW, pCloneToView));

			_tabPopupMenu.create(_hSelf, itemUnitArray);
			
		}
		::ScreenToClient(_pDocTab->getHSelf(), &clientPoint);
        ::SendMessage(_pDocTab->getHSelf(), WM_LBUTTONDOWN, 2, MAKELONG(clientPoint.x, clientPoint.y));

		bool isEnable = ((::GetMenuState(_mainMenuHandle, IDM_FILE_SAVE, MF_BYCOMMAND)&MF_DISABLED) == 0);
		_tabPopupMenu.enableItem(IDM_FILE_SAVE, isEnable);
		
		bool isUserReadOnly = _pEditView->isCurrentBufUserReadOnly();
		_tabPopupMenu.checkItem(IDM_EDIT_SETREADONLY, isUserReadOnly);

		bool isSysReadOnly = _pEditView->isCurrentBufSysReadOnly();
		_tabPopupMenu.enableItem(IDM_EDIT_SETREADONLY, !isSysReadOnly);
		_tabPopupMenu.enableItem(IDM_EDIT_CLEARREADONLY, isSysReadOnly);

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
        else if (notification->margin == ScintillaEditView::_SC_MARGE_SYBOLE)
        {
            
            int lineClick = int(_pEditView->execute(SCI_LINEFROMPOSITION, notification->position));
			if (!showLines(lineClick))
				bookmarkToggle(lineClick);
        
        }
		break;
	}
	
	case SCN_CHARADDED:
	{
		charAdded(static_cast<char>(notification->ch));
		static const NppGUI & nppGUI = NppParameters::getInstance()->getNppGUI();

		char s[64];
		notifyView->getWordToCurrentPos(s, sizeof(s));
		
		if (strlen(s) >= nppGUI._autocFromLen)
		{
			if (nppGUI._autocStatus == nppGUI.autoc_word)
				autoCompFromCurrentFile(false);
			else if (nppGUI._autocStatus == nppGUI.autoc_func)
				showAutoComp();
		}
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
		else
		{
			markSelectedText();
		}
	}
	break;

    case SCN_UPDATEUI:
        braceMatch();
		markSelectedText();
		updateStatusBar();
        break;

    case TTN_GETDISPINFO: 
    { 
        LPTOOLTIPTEXT lpttt; 

        lpttt = (LPTOOLTIPTEXT)notification; 
        lpttt->hinst = _hInst; 

        // Specify the resource identifier of the descriptive 
        // text for the given button. 
        int idButton = int(lpttt->hdr.idFrom);
		static string tip;
		getNameStrFromCmd(idButton, tip);
		lpttt->lpszText = (LPSTR)tip.c_str();
    } 
    break;

    case SCN_ZOOM:
        notifyView->setLineNumberWidth(_pEditView->hasMarginShowed(ScintillaEditView::_SC_MARGE_LINENUMBER));
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

		char currentWord[MAX_PATH*2];
		notifyView->getText(currentWord, startPos, endPos);

		::ShellExecute(_hSelf, "open", currentWord, NULL, NULL, SW_SHOW);
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
	char charBefore = '\0';
	//char styleBefore = '\0';
	int lengthDoc = int(_pEditView->execute(SCI_GETLENGTH));

	if ((lengthDoc > 0) && (caretPos > 0)) 
    {
		charBefore = char(_pEditView->execute(SCI_GETCHARAT, caretPos - 1, 0));
	}
	// Priority goes to character before caret
	if (charBefore && strchr("[](){}", charBefore))
    {
		braceAtCaret = caretPos - 1;
	}

	if (lengthDoc > 0  && (braceAtCaret < 0)) 
    {
		// No brace found so check other side
		char charAfter = char(_pEditView->execute(SCI_GETCHARAT, caretPos, 0));
		if (charAfter && strchr("[](){}", charAfter))
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

void Notepad_plus::charAdded(char chAdded)
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
		unsigned char ch = (unsigned char)_pEditView->execute(SCI_GETCHARAT, posBegin2style);

		// determinating the type of EOF to make sure how many steps should we be back
		if ((ch == 0x0A) || (ch == 0x0D))
		{
			int eolMode = _pEditView->execute(SCI_GETEOLMODE);
			
			if ((eolMode == SC_EOL_CRLF) && (posBegin2style > 1))
				posBegin2style -= 2;
			else if (posBegin2style > 0)
				posBegin2style -= 1;
		}

		ch = (unsigned char)_pEditView->execute(SCI_GETCHARAT, posBegin2style);
		while ((posBegin2style > 0) && ((ch != 0x0A) && (ch != 0x0D)))
		{
			ch = (unsigned char)_pEditView->execute(SCI_GETCHARAT, posBegin2style--);
		}
	}
	int style_hotspot = 30;

	int startPos = 0;
	int endPos = _pEditView->execute(SCI_GETTEXTLENGTH);

	_pEditView->execute(SCI_SETSEARCHFLAGS, SCFIND_REGEXP|SCFIND_POSIX);

	_pEditView->execute(SCI_SETTARGETSTART, startPos);
	_pEditView->execute(SCI_SETTARGETEND, endPos);

	vector<pair<int, int> > hotspotStylers;
	
	//char *regExprStr0 = "http://[a-z0-9_-+.:?=/%]*";//"http://[^ \\t\\\"]*";
	//char *regExprStr1 = "[a-zA-Z0-9._]+@[a-zA-Z0-9_]+.[a-zA-Z0-9_]+";

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

				char fontName[256];
				Style hotspotStyle;
				
				_pEditView->execute(SCI_STYLEGETFONT, idStyle, (LPARAM)fontName);
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



void Notepad_plus::MaintainIndentation(char ch)
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
			nbColumnEdgeDlg.init(_hInst, _hSelf, svp._edgeNbColumn, "Nb of column:");
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
			fileNew();
			break;
		
		case IDM_FILE_OPEN:
			fileOpen();
			break;

		case IDM_FILE_RELOAD:
			fileReload();
			break;

		case IDM_FILE_CLOSE:
			fileClose();
			checkSyncState();
			break;

		case IDM_FILE_CLOSEALL:
			fileCloseAll();
			checkSyncState();
			break;

		case IDM_FILE_CLOSEALL_BUT_CURRENT :
			fileCloseAllButCurrent();
			checkSyncState();
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
		{
			str2Cliboard(_pEditView->getCurrentTitle());
		}
		break;

		case IDM_EDIT_CURRENTDIRTOCLIP :
		{
			char dir[MAX_PATH];
			strcpy(dir, _pEditView->getCurrentTitle());
			PathRemoveFileSpec((LPSTR)dir);

			str2Cliboard(dir);
		}
		break;

		case IDM_EDIT_FILENAMETOCLIP :
		{
			str2Cliboard(PathFindFileName((LPSTR)_pEditView->getCurrentTitle()));
		}
		break;

		case IDM_SEARCH_FIND :
		case IDM_SEARCH_REPLACE :
		{
			const int strSize = 64;
			char str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();

			if (_nativeLang)
			{
				TiXmlNode *dlgNode = _nativeLang->FirstChild("Dialog");
				if (dlgNode)
				{
					dlgNode = searchDlgNode(dlgNode, "Find");
					if (dlgNode)
					{
						const char *titre1 = (dlgNode->ToElement())->Attribute("titleFind");
						const char *titre2 = (dlgNode->ToElement())->Attribute("titleReplace");
						const char *titre3 = (dlgNode->ToElement())->Attribute("titleFindInFiles");
						if (titre1 && titre2 && titre3)
						{
							pNppParam->getFindDlgTabTitiles()._find = titre1;
							pNppParam->getFindDlgTabTitiles()._replace = titre2;
							pNppParam->getFindDlgTabTitiles()._findInFiles = titre3;
						}
					}
				}
			}
			_findReplaceDlg.doDialog((id == IDM_SEARCH_FIND)?FIND_DLG:REPLACE_DLG, _isRTL);

			_pEditView->getSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str, _pEditView->getCurrentBuffer()._unicodeMode != uni8Bit);

			if (isFirstTime)
				changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
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
			char str[strSize];

			_incrementFindDlg.display();

			_pEditView->getSelectedText(str, strSize);
			_incrementFindDlg.setSearchText(str, _pEditView->getCurrentBuffer()._unicodeMode != uni8Bit);
		}
		break;

		case IDM_SEARCH_FINDNEXT :
		case IDM_SEARCH_FINDPREV :
		{
			if (!_findReplaceDlg.isCreated())
				return;

			_findReplaceDlg.setSearchDirection(id == IDM_SEARCH_FINDNEXT?DIR_DOWN:DIR_UP);

			string s = _findReplaceDlg.getText2search();
			_findReplaceDlg.processFindNext(s.c_str());
			break;
		}
		break;

		case IDM_SEARCH_VOLATILE_FINDNEXT :
		case IDM_SEARCH_VOLATILE_FINDPREV :
		{
			char text2Find[MAX_PATH];
			_pEditView->getSelectedText(text2Find, sizeof(text2Find));

			FindOption op;
			op._isWholeWord = false;
			op._whichDirection = (id == IDM_SEARCH_VOLATILE_FINDNEXT?DIR_DOWN:DIR_UP);
			_findReplaceDlg.processFindNext(text2Find, &op);
			break;
		}
		case IDM_SEARCH_MARKALL :
		{
			const int strSize = 64;
			char text2Find[strSize];
			_pEditView->getSelectedText(text2Find, sizeof(text2Find));

			FindOption op;
			op._isWholeWord = false;
			//op._whichDirection = (id == IDM_SEARCH_VOLATILE_FINDNEXT?DIR_DOWN:DIR_UP);
			_findReplaceDlg.markAll(text2Find);

			break;
		}

		case IDM_SEARCH_UNMARKALL :
		{
			LangType lt = _pEditView->getCurrentDocType();
            if (lt == L_TXT)
                _pEditView->defineDocType(L_CPP); 
			_pEditView->defineDocType(lt);
			_pEditView->execute(SCI_MARKERDELETEALL, MARK_BOOKMARK);
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

					if (_mainWindowStatus & TWO_VIEWS_MASK)
						_pMainWindow = &_subSplitter;
					else
						_pMainWindow = _pDocTab;

					::SendMessage(_hSelf, WM_SIZE, 0, 0);
					
					udd->display(false);
					_mainWindowStatus &= ~DOCK_MASK;
				}
				else if ((isUDDlgDocked)&&(!isUDDlgVisible))
				{
                    if (!_pMainSplitter)
                    {
                        _pMainSplitter = new SplitterContainer;
                        _pMainSplitter->init(_hInst, _hSelf);

                        Window *pWindow;
                        if (_mainWindowStatus & TWO_VIEWS_MASK)
                            pWindow = &_subSplitter;
                        else
                            pWindow = _pDocTab;

                        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, RIGHT_FIX, 45); 
                    }

					_pMainWindow = _pMainSplitter;

					_pMainSplitter->setWin0((_mainWindowStatus & TWO_VIEWS_MASK)?(Window *)&_subSplitter:(Window *)_pDocTab);

					::SendMessage(_hSelf, WM_SIZE, 0, 0);
					_pMainWindow->display();

					_mainWindowStatus |= DOCK_MASK;
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
			int check = (::GetMenuState(_mainMenuHandle, id, MF_BYCOMMAND) == MF_CHECKED)?MF_UNCHECKED:MF_CHECKED;
			::CheckMenuItem(_mainMenuHandle, id, MF_BYCOMMAND | check);
			_pEditView->setCurrentDocReadOnlyByUser(check == MF_CHECKED);
			_pDocTab->updateCurrentTabItem();
		}
		break;

		case IDM_EDIT_CLEARREADONLY:
		{
			DWORD dwFileAttribs = ::GetFileAttributes(_pEditView->getCurrentBuffer().getFileName());
			dwFileAttribs ^= FILE_ATTRIBUTE_READONLY; 

			::SetFileAttributes(_pEditView->getCurrentBuffer().getFileName(), dwFileAttribs); 

			_pEditView->execute(SCI_SETREADONLY,false);
			_pEditView->updateCurrentDocSysReadOnlyStat();

			_pDocTab->updateCurrentTabItem();
			enableCommand(IDM_EDIT_SETREADONLY, true, MENU);
		}
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

		case IDM_VIEW_TOOLBAR_HIDE:
		{
			bool toSet = !_rebarTop.getIDVisible(REBAR_BAR_TOOLBAR);
			_rebarTop.setIDVisible(REBAR_BAR_TOOLBAR, toSet);
		}
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

        case IDM_VIEW_STATUSBAR:
		{
            RECT rc;
			getClientRect(rc);
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			nppGUI._statusBarShow = !nppGUI._statusBarShow;
            _statusBar.display(nppGUI._statusBarShow);
            ::SendMessage(_hSelf, WM_SIZE, SIZE_RESTORED, MAKELONG(rc.bottom, rc.right));
            break;
        }

		case IDM_VIEW_HIDEMENU :
		{
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();
			nppGUI._menuBarShow = !nppGUI._menuBarShow;
			if (nppGUI._menuBarShow)
				::SetMenu(_hSelf, _mainMenuHandle);
			else
				::SetMenu(_hSelf, NULL);
			break;
		}

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
			CharacterRange range = _pEditView->getSelection();
			int startLine = _pEditView->execute(SCI_LINEFROMPOSITION, range.cpMin);
			int endLine = _pEditView->execute(SCI_LINEFROMPOSITION, range.cpMax);

			if (startLine == 0)
				startLine = 1;
			if (endLine == _pEditView->getNbLine())
				endLine -= 1;
			_pEditView->execute(SCI_HIDELINES, startLine, endLine);
			_pEditView->execute(SCI_MARKERADD, startLine-1, MARK_HIDELINESBEGIN);
			_pEditView->execute(SCI_MARKERADD, endLine+1, MARK_HIDELINESEND);
			_hideLinesMarks.push_back(pair<int, int>(startLine-1, endLine+1));
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
			_pEditView->execute(SCI_SETZOOM, _zoomOriginalValue);
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
			int f = int((id == IDM_FORMAT_TODOS)?SC_EOL_CRLF:(id == IDM_FORMAT_TOUNIX)?SC_EOL_LF:SC_EOL_CR);
			_pEditView->execute(SCI_SETEOLMODE, f);
			_pEditView->execute(SCI_CONVERTEOLS, f);
			(_pEditView->getCurrentBuffer()).setFormat((formatType)f);
			enableConvertMenuItems((formatType)f);
			setDisplayFormat((formatType)f);
			break;
		}

		case IDM_FORMAT_ANSI :
		case IDM_FORMAT_UTF_8 :	
		case IDM_FORMAT_UCS_2BE :
		case IDM_FORMAT_UCS_2LE :
		case IDM_FORMAT_AS_UTF_8 :
		{
			UniMode um;
			switch (id)
			{
				case IDM_FORMAT_AS_UTF_8:
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
					um = uni8Bit;
			}
			if (_pEditView->getCurrentBuffer().getUnicodeMode() != um)
			{
				_pEditView->getCurrentBuffer().setUnicodeMode(um);
				_pDocTab->updateCurrentTabItem();
				checkDocState();
				synchronise();

				_pEditView->execute(SCI_SETCODEPAGE, (um != uni8Bit)?SC_CP_UTF8:0);
				checkUnicodeMenuItems(um);
				setUniModeText(um);
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
			Buffer & currentBuffer = _pEditView->getCurrentBuffer();
			UniMode um = currentBuffer._unicodeMode;

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
				//::GlobalFree(allocClipboardData);
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
			tabSizeDlg.init(_hInst, _hSelf, nppgui._tabSize, "Tab Size : ");
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
			valDlg.init(_hInst, _hSelf, nppGUI._autocFromLen, "Nb char : ");
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
			nbHistoryDlg.init(_hInst, _hSelf, pNppParam->getNbMaxFile(), "Max File : ");
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
            docGotoAnotherEditView(MODE_TRANSFER);
			checkSyncState();
            break;

        case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
            docGotoAnotherEditView(MODE_CLONE);
			checkSyncState();
            break;

        case IDM_ABOUT:
		{
			bool isFirstTime = !_aboutDlg.isCreated();
            _aboutDlg.doDialog();
			if (isFirstTime && _nativeLang)
			{
				const char *lang = (_nativeLang->ToElement())->Attribute("name");
				if (lang && !strcmp(lang, "中文繁體"))
				{
					char *authorName = "侯今吾";
					HWND hItem = ::GetDlgItem(_aboutDlg.getHSelf(), IDC_AUTHOR_NAME);
					::SetWindowText(hItem, authorName);
				}
			}
			break;
		}

		case IDM_HOMESWEETHOME :
		{
			::ShellExecute(NULL, "open", "http://notepad-plus.sourceforge.net/", NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		case IDM_PROJECTPAGE :
		{
			::ShellExecute(NULL, "open", "http://sourceforge.net/projects/notepad-plus/", NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_ONLINEHELP:
		{
			::ShellExecute(NULL, "open", "http://notepad-plus.sourceforge.net/uk/generalFAQ.php", NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_WIKIFAQ:
		{
			::ShellExecute(NULL, "open", "http://notepad-plus.wiki.sourceforge.net/FAQ", NULL, NULL, SW_SHOWNORMAL);
			break;
		}
		
		case IDM_FORUM:
		{
			::ShellExecute(NULL, "open", "http://sourceforge.net/forum/?group_id=95717", NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_PLUGINSHOME:
		{
			::ShellExecute(NULL, "open", "https://sourceforge.net/projects/npp-plugins/", NULL, NULL, SW_SHOWNORMAL);
			break;
		}

		case IDM_UPDATE_NPP :
		{
			string updaterDir = pNppParam->getNppPath();
			updaterDir += "\\updater\\";
			string updaterFullPath = updaterDir + "gup.exe";
			string param = "-verbose -v";
			param += VERSION_VALUE;
			Process updater(updaterFullPath.c_str(), param.c_str(), updaterDir.c_str());
			updater.run();
			break;
		}

		case IDM_EDIT_AUTOCOMPLETE :
			showAutoComp();
			break;

		case IDM_EDIT_AUTOCOMPLETE_CURRENTFILE :
			//MessageBox(NULL, "IDM_EDIT_AUTOCOMPLETE_CURRENTFILE", "", MB_OK);
			autoCompFromCurrentFile();
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
            setLanguage(id, L_C); 
            break;

        case IDM_LANG_CPP :
            setLanguage(id, L_CPP); 
            break;

        case IDM_LANG_JAVA :
            setLanguage(id, L_JAVA); 
            break;

        case IDM_LANG_CS :
            setLanguage(id, L_CS); 
            break;

        case IDM_LANG_HTML :
            setLanguage(id, L_HTML); 
            break;

        case IDM_LANG_XML :
            setLanguage(id, L_XML); 
            break;

        case IDM_LANG_JS :
            setLanguage(id, L_JS); 
            break;

        case IDM_LANG_PHP :
            setLanguage(id, L_PHP); 
            break;

        case IDM_LANG_ASP :
            setLanguage(id, L_ASP); 
            break;

        case IDM_LANG_CSS :
            setLanguage(id, L_CSS); 
            break;

        case IDM_LANG_LUA :
            setLanguage(id, L_LUA); 
            break;

        case IDM_LANG_PERL :
            setLanguage(id, L_PERL); 
            break;

        case IDM_LANG_PYTHON :
            setLanguage(id, L_PYTHON); 
            break;

        case IDM_LANG_PASCAL :
            setLanguage(id, L_PASCAL); 
            break;

        case IDM_LANG_BATCH :
            setLanguage(id, L_BATCH); 
            break;

        case IDM_LANG_OBJC :
            setLanguage(id, L_OBJC); 
            break;

        case IDM_LANG_VB :
            setLanguage(id, L_VB); 
            break;

        case IDM_LANG_SQL :
            setLanguage(id, L_SQL); 
            break;

        case IDM_LANG_ASCII :
            setLanguage(id, L_NFO); 
            break;

        case IDM_LANG_TEXT :
            setLanguage(id, L_TXT); 
            break;

        case IDM_LANG_RC :
            setLanguage(id, L_RC); 
            break;

        case IDM_LANG_MAKEFILE :
            setLanguage(id, L_MAKEFILE); 
            break;

        case IDM_LANG_INI :
            setLanguage(id, L_INI); 
            break;

        case IDM_LANG_TEX :
            setLanguage(id, L_TEX); 
            break;

        case IDM_LANG_FORTRAN :
            setLanguage(id, L_FORTRAN); 
            break;

        case IDM_LANG_SH :
            setLanguage(id, L_BASH); 
            break;

        case IDM_LANG_FLASH :
            setLanguage(id, L_FLASH); 
            break;

		case IDM_LANG_NSIS :
            setLanguage(id, L_NSIS); 
            break;

		case IDM_LANG_TCL :
            setLanguage(id, L_TCL); 
            break;

		case IDM_LANG_LISP :
			setLanguage(id, L_LISP); 
			break;

		case IDM_LANG_SCHEME :
			setLanguage(id, L_SCHEME); 
			break;

		case IDM_LANG_ASM :
            setLanguage(id, L_ASM);
			break;

		case IDM_LANG_DIFF :
            setLanguage(id, L_DIFF);
			break;

		case IDM_LANG_PROPS :
            setLanguage(id, L_PROPS);
			break;

		case IDM_LANG_PS:
            setLanguage(id, L_PS);
			break;

		case IDM_LANG_RUBY:
            setLanguage(id, L_RUBY);
			break;

		case IDM_LANG_SMALLTALK:
            setLanguage(id, L_SMALLTALK);
			break;
		case IDM_LANG_VHDL :
            setLanguage(id, L_VHDL);
			break;

        case IDM_LANG_KIX :
            setLanguage(id, L_KIX); 
            break;

        case IDM_LANG_CAML :
            setLanguage(id, L_CAML); 
            break;

        case IDM_LANG_ADA :
            setLanguage(id, L_ADA); 
            break;

        case IDM_LANG_VERILOG :
            setLanguage(id, L_VERILOG); 
            break;

		case IDM_LANG_MATLAB :
            setLanguage(id, L_MATLAB); 
            break;

		case IDM_LANG_HASKELL :
            setLanguage(id, L_HASKELL); 
            break;

        case IDM_LANG_AU3 :
            setLanguage(id, L_AU3); 
            break;

		case IDM_LANG_INNO :
            setLanguage(id, L_INNO); 
            break;

		case IDM_LANG_CMAKE :
            setLanguage(id, L_CMAKE); 
            break;

		case IDM_LANG_YAML :
			setLanguage(id, L_YAML); 
            break;

		case IDM_LANG_USER :
            setLanguage(id, L_USER); 
            break;

        case IDC_PREV_DOC :
        case IDC_NEXT_DOC :
        {
			int nbDoc = _mainDocTab.isVisible()?_mainEditView.getNbDoc():0;
			nbDoc += _subDocTab.isVisible()?_subEditView.getNbDoc():0;
			
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

		case IDM_OPEN_ALL_RECENT_FILE :
			for (int i = IDM_FILEMENU_LASTONE + 1 ; i < (IDM_FILEMENU_LASTONE + _lastRecentFileList.getMaxNbLRF() + 1) ; i++)
			{
				char fn[MAX_PATH];
				int res = ::GetMenuString(_mainMenuHandle, i, fn, sizeof(fn), MF_BYCOMMAND);
				if (res)
				{
					doOpen(fn);
				}
			}
			break;

		case IDM_CLEAN_RECENT_FILE_LIST :
			for (int i = IDM_FILEMENU_LASTONE + 1 ; i < (IDM_FILEMENU_LASTONE + _lastRecentFileList.getMaxNbLRF() + 1) ; i++)
			{
				char fn[MAX_PATH];
				int res = ::GetMenuString(_mainMenuHandle, i, fn, sizeof(fn), MF_BYCOMMAND);
				if (res)
				{
					_lastRecentFileList.remove(fn);
				}
			}
			break;

		case IDM_EDIT_RTL :
		case IDM_EDIT_LTR :
		{
			long exStyle = ::GetWindowLong(_pEditView->getHSelf(), GWL_EXSTYLE);
			exStyle = (id == IDM_EDIT_RTL)?exStyle|WS_EX_LAYOUTRTL:exStyle&(~WS_EX_LAYOUTRTL);
			::SetWindowLong(_pEditView->getHSelf(), GWL_EXSTYLE, exStyle);
			_pEditView->defineDocType(_pEditView->getCurrentDocType());
			_pEditView->redraw();
		}
		break;

		case IDM_WINDOW_WINDOWS :
		{
			WindowsDlg _windowsDlg;
			_windowsDlg.init(_hInst, _hSelf, _pEditView);
			
			TiXmlNode *dlgNode = NULL;
			if (_nativeLang)
			{
				dlgNode = _nativeLang->FirstChild("Dialog");
				if (dlgNode)
					dlgNode = searchDlgNode(dlgNode, "Window");
			}
			_windowsDlg.doDialog(dlgNode);

			//changeDlgLang(_windowsDlg.getHSelf(), "Window");
		}
		break;

		default :
			if (id > IDM_FILE_EXIT && id < (IDM_FILE_EXIT + _lastRecentFileList.getMaxNbLRF() + 1))
			{
				char fn[MAX_PATH];
				int res = ::GetMenuString(_mainMenuHandle, id, fn, sizeof(fn), MF_BYCOMMAND);
				if (res)
				{
					if (doOpen(fn))
					{
						setLangStatus(_pEditView->getCurrentDocType());	
					}
				}
			}
			else if ((id > IDM_LANG_USER) && (id < IDM_LANG_USER_LIMIT))
			{
				char langName[langNameLenMax];
				::GetMenuString(_mainMenuHandle, id, langName, sizeof(langName), MF_BYCOMMAND);
				_pEditView->setCurrentDocUserType(langName);
				setLangStatus(L_USER);
				checkLangsMenu(id);
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

void Notepad_plus::setTitleWith(const char *filePath)
{
	if (!filePath || !strcmp(filePath, ""))
		return;

	const size_t str2concatLen = MAX_PATH + 32;
	char str2concat[str2concatLen]; 
	strcat(strcpy(str2concat, filePath), " - ");
	strcat(str2concat, _className);
	::SetWindowText(_hSelf, str2concat);
}

void Notepad_plus::activateNextDoc(bool direction) 
{
    int nbDoc = _pEditView->getNbDoc();
    if (!nbDoc) return;

    int curIndex = _pEditView->getCurrentDocIndex();
    curIndex += (direction == dirUp)?-1:1;

	if (curIndex >= nbDoc)
	{
		if (getNonCurrentDocTab()->isVisible())
			switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
		curIndex = 0;
	}
	else if (curIndex < 0)
	{
		if (getNonCurrentDocTab()->isVisible())
		{
			switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
			nbDoc = _pEditView->getNbDoc();
		}
		curIndex = nbDoc - 1;
	}

	char *fullPath = _pDocTab->activate(curIndex);
    setTitleWith(fullPath);
	//checkDocState();

	char dirPath[MAX_PATH];

	strcpy(dirPath, fullPath);
	PathRemoveFileSpec(dirPath);
	setWorkingDir(dirPath);
}


void Notepad_plus::activateDoc(int pos)
{
	int nbDoc = _pEditView->getNbDoc();
	if (!nbDoc) return;

	if (pos == _pEditView->getCurrentDocIndex())
	{
		(_pEditView->getCurrentBuffer()).increaseRecentTag();
		return;
	}

	if (pos >= 0 && pos < nbDoc)
	{
		char *fullPath = _pDocTab->activate(pos);
		setTitleWith(fullPath);
		//checkDocState();
		char dirPath[MAX_PATH];

		strcpy(dirPath, fullPath);
		PathRemoveFileSpec(dirPath);
		setWorkingDir(dirPath);
	}
}

void Notepad_plus::updateStatusBar() 
{
    char strLnCol[64];
	sprintf(strLnCol, "Ln : %d    Col : %d    Sel : %d",\
        (_pEditView->getCurrentLineNumber() + 1), \
		(_pEditView->getCurrentColumnNumber() + 1),\
		(_pEditView->getSelectedByteNumber()));

    _statusBar.setText(strLnCol, STATUSBAR_CUR_POS);

	char strDonLen[64];
	sprintf(strDonLen, "nb char : %d", _pEditView->getCurrentDocLen());
	_statusBar.setText(strDonLen, STATUSBAR_DOC_SIZE);

	setDisplayFormat((_pEditView->getCurrentBuffer()).getFormat());
	setUniModeText(_pEditView->getCurrentBuffer().getUnicodeMode());
    _statusBar.setText(_pEditView->execute(SCI_GETOVERTYPE) ? "OVR" : "INS", STATUSBAR_TYPING_MODE);
}


void Notepad_plus::dropFiles(HDROP hdrop) 
{
	if (hdrop)
	{
		// Determinate in which view the file(s) is (are) dropped
		POINT p;
		::DragQueryPoint(hdrop, &p);
		HWND hWin = ::ChildWindowFromPoint(_hSelf, p);
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
		for (int i = 0 ; i < filesDropped ; ++i)
		{
			char pathDropped[MAX_PATH];
			::DragQueryFile(hdrop, i, pathDropped, sizeof(pathDropped));
			doOpen(pathDropped);
            //setLangStatus(_pEditView->getCurrentDocType());
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
	const int NB_VIEW = 2;
	struct ViewInfo {
		int id;
		ScintillaEditView * sv;
		DocTabView * dtv;
		int currentIndex;
		bool toBeActivated;
	};

	ViewInfo viewInfoArray[NB_VIEW];
	
	// the oder (1.current view 2.non current view) is important
	// to synchronize with "hideCurrentView" function
	viewInfoArray[0].id = getCurrentView();
	viewInfoArray[0].sv = _pEditView;
	viewInfoArray[0].dtv = _pDocTab;
	viewInfoArray[0].currentIndex = _pEditView->getCurrentDocIndex();
	viewInfoArray[0].toBeActivated = false;

	viewInfoArray[1].id = getNonCurrentView();
	viewInfoArray[1].sv = getNonCurrentEditView();
	viewInfoArray[1].dtv = getNonCurrentDocTab();
	viewInfoArray[1].currentIndex = viewInfoArray[1].sv->getCurrentDocIndex();
	viewInfoArray[1].toBeActivated = false;

	NppParameters *pNppParam = NppParameters::getInstance();
	const NppGUI & nppGUI = pNppParam->getNppGUI();
	bool autoUpdate = (nppGUI._fileAutoDetection == cdAutoUpdate) || (nppGUI._fileAutoDetection == cdAutoUpdateGo2end);

	_subEditView.getCurrentDocIndex();
	for (int j = 0 ; j < NB_VIEW ; j++)
	{
		ViewInfo & vi = viewInfoArray[j];
		if (vi.sv->isVisible())
		{
			for (int i = ((vi.sv)->getNbDoc()-1) ; i >= 0  ; i--)
			{
				Buffer & docBuf = vi.sv->getBufferAt(i);
				docFileStaus fStatus = docBuf.checkFileState();
         		bool update = !docBuf.isDirty() && autoUpdate;

				if (fStatus == MODIFIED_FROM_OUTSIDE)
				{
					// If npp is minimized, bring it up to the top
					if (::IsIconic(_hSelf))
						::ShowWindow(_hSelf, SW_SHOWNORMAL);

					if (update)
					{
						docBuf._reloadOnSwitchBack = true;
						// for 2 views, if it's current doc, then reload immediately
						if (vi.currentIndex == i)
						{
							vi.toBeActivated = true;
							if (j == 0) // 0 == current view
							{
								_activeAppInf._isActivated = false;
							}
						}
					}
					else if (doReloadOrNot(docBuf.getFileName()) == IDYES)
					{
						docBuf._reloadOnSwitchBack = true;
						setTitleWith(vi.dtv->activate(i));
						// if it's a non current view, make it as the current view
						if (j == 1)
							switchEditViewTo(getNonCurrentView());
					}

					if (_activeAppInf._isActivated)
					{
						int curPos = _pEditView->execute(SCI_GETCURRENTPOS);
						::PostMessage(_pEditView->getHSelf(), WM_LBUTTONUP, 0, 0);
						::PostMessage(_pEditView->getHSelf(), SCI_SETSEL, curPos, curPos);
						_activeAppInf._isActivated = false;
					}
					docBuf.updatTimeStamp();
				}
				else if (fStatus == FILE_DELETED && !docBuf._dontBotherMeAnymore)
				{
					if (::IsIconic(_hSelf))
						::ShowWindow(_hSelf, SW_SHOWNORMAL);

					if (doCloseOrNot(docBuf.getFileName()) == IDNO)
					{
						vi.dtv->activate(i);
						if ((vi.sv->getNbDoc() == 1) && (_mainWindowStatus & TWO_VIEWS_MASK))
						{
							setTitleWith(vi.dtv->closeCurrentDoc());
							hideCurrentView();
						}
						else
							setTitleWith(vi.dtv->closeCurrentDoc());
					}
					else
						docBuf._dontBotherMeAnymore = true;

					if (_activeAppInf._isActivated)
					{
						int curPos = _pEditView->execute(SCI_GETCURRENTPOS);
						::PostMessage(_pEditView->getHSelf(), WM_LBUTTONUP, 0, 0);
						::PostMessage(_pEditView->getHSelf(), SCI_SETSEL, curPos, curPos);
						_activeAppInf._isActivated = false;
					}
				}
		        
				bool isReadOnly = vi.sv->isCurrentBufReadOnly();
				vi.sv->execute(SCI_SETREADONLY, isReadOnly);
			}
		}
	}

	if (autoUpdate)
	{
		if (viewInfoArray[0].toBeActivated)
		{
			switchEditViewTo(viewInfoArray[0].id);
		}
		
		if (viewInfoArray[1].toBeActivated)
		{
			switchEditViewTo(viewInfoArray[1].id);
			switchEditViewTo(viewInfoArray[0].id);
		}
	}
}

void Notepad_plus::reloadOnSwitchBack()
{
	Buffer & buf = _pEditView->getCurrentBuffer();

	if (buf._reloadOnSwitchBack)
	{
		if (_pEditView->isCurrentBufReadOnly())
			_pEditView->execute(SCI_SETREADONLY, FALSE);

		reload(buf.getFileName());

		NppParameters *pNppParam = NppParameters::getInstance();
		const NppGUI & nppGUI = pNppParam->getNppGUI();
		if (nppGUI._fileAutoDetection == cdAutoUpdateGo2end || nppGUI._fileAutoDetection == cdGo2end)
		{
			int line = _pEditView->getNbLine();
			_pEditView->gotoLine(line);
		}

		if (_pEditView->isCurrentBufReadOnly())
			_pEditView->execute(SCI_SETREADONLY, TRUE);

		buf._reloadOnSwitchBack = false;
	}
}

void Notepad_plus::hideCurrentView()
{
	if (_mainWindowStatus & DOCK_MASK)
	{
		_pMainSplitter->setWin0(getNonCurrentDocTab());
	}
	else // otherwise the main window is the spltter container that we just created
		_pMainWindow = getNonCurrentDocTab();
	    
	_subSplitter.display(false);
	_pEditView->display(false);
	_pDocTab->display(false);

	// resize the main window
	::SendMessage(_hSelf, WM_SIZE, 0, 0);

	switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);

	//setTitleWith(_pEditView->getCurrentTitle());

	_mainWindowStatus &= ~TWO_VIEWS_MASK;
}

bool Notepad_plus::fileClose()
{
	// Notify plugins that current file is about to be closed
	SCNotification scnN;
	scnN.nmhdr.code = NPPN_FILEBEFORECLOSE;
	scnN.nmhdr.hwndFrom = _hSelf;
	scnN.nmhdr.idFrom = 0;
	_pluginsManager.notify(&scnN);

	int res;
	bool isDirty = _pEditView->isCurrentDocDirty();
	
	//process the fileNamePath into LRF
	const char *fileNamePath = _pEditView->getCurrentTitle();
	
	if ((!isDirty) && (Buffer::isUntitled(fileNamePath)) && (_pEditView->getNbDoc() == 1) && (!getNonCurrentDocTab()->isVisible()))
		return true;
		
	if (isDirty)
	{
		if ((res = doSaveOrNot(_pEditView->getCurrentTitle())) == IDYES)
		{
			if (!fileSave()) // the cancel button of savdialog is pressed
				return false;
		}
		else if (res == IDCANCEL)
			return false;
		// else IDNO we continue
	}

	//si ce n'est pas untited(avec prefixe "new "), on fait le traitement
	if (!Buffer::isUntitled(fileNamePath))
	{
		_lastRecentFileList.add(fileNamePath);
	}


	if ((_pEditView->getNbDoc() == 1) && (_mainWindowStatus & TWO_VIEWS_MASK))
	{
		_pDocTab->closeCurrentDoc();
		hideCurrentView();
		scnN.nmhdr.code = NPPN_FILECLOSED;
		_pluginsManager.notify(&scnN);
		return true;
	}

	char fullPath[MAX_PATH];
	strcpy(fullPath, _pDocTab->closeCurrentDoc());
	setTitleWith(fullPath);
	
	PathRemoveFileSpec(fullPath);
	setWorkingDir(fullPath);

	//updateStatusBar();
	//dynamicCheckMenuAndTB();
	//setLangStatus(_pEditView->getCurrentDocType());
	//checkDocState();
	_linkTriggered = true;
	::SendMessage(_hSelf, NPPM_INTERNAL_DOCSWITCHIN, 0, 0);

	// Notify plugins that current file is closed
	scnN.nmhdr.code = NPPN_FILECLOSED;
	_pluginsManager.notify(&scnN);
	
	return true;
}

bool Notepad_plus::fileCloseAll()
{
    if (_mainWindowStatus & TWO_VIEWS_MASK)
    {
        while (_pEditView->getNbDoc() > 1)
		    if (!fileClose())
			    return false;

	    if (!fileClose())
			return false;
    }

	while (_pEditView->getNbDoc() > 1)
		if (!fileClose())
			return false;
	return fileClose();
}

bool Notepad_plus::fileCloseAllButCurrent()
{
	int curIndex = _pEditView->getCurrentDocIndex();
	_pEditView->activateDocAt(0);

	for (int i = 0 ; i < curIndex ; i++)
		if (!fileClose())
		{
			curIndex -= i;
			_pDocTab->activate(curIndex);
			return false;
		}

	if (_pEditView->getNbDoc() > 1)
	{
		_pDocTab->activate(1);
		while (_pEditView->getNbDoc() > 1)
			if (!fileClose())
			    return false;
	}
	if (_mainWindowStatus & TWO_VIEWS_MASK)
	{
		switchEditViewTo(getNonCurrentView());
		while (_pEditView->getNbDoc() > 1)
			if (!fileClose())
				return false;
		return fileClose();
	}
	return true;
}

void Notepad_plus::reload(const char *fileName)
{
	Utf8_16_Read UnicodeConvertor;
	Buffer & buffer = _pEditView->getCurrentBuffer();

	FILE *fp = fopen(fileName, "rb");
	if (fp)
	{
		// It's VERY IMPORTANT to reset the view
		_pEditView->execute(SCI_CLEARALL);

		char data[blockSize];

		size_t lenFile = fread(data, 1, sizeof(data), fp);
		while (lenFile > 0) {
			lenFile = UnicodeConvertor.convert(data, lenFile);
			_pEditView->execute(SCI_ADDTEXT, lenFile, reinterpret_cast<LPARAM>(UnicodeConvertor.getNewBuf()));
			lenFile = int(fread(data, 1, sizeof(data), fp));
		}
		fclose(fp);

		UniMode unicodeMode = static_cast<UniMode>(UnicodeConvertor.getEncoding());
		buffer.setUnicodeMode(unicodeMode);

		if (unicodeMode != uni8Bit)
			// Override the code page if Unicode
			_pEditView->execute(SCI_SETCODEPAGE, SC_CP_UTF8);

		_pEditView->getFocus();
		_pEditView->execute(SCI_SETSAVEPOINT);
		_pEditView->execute(EM_EMPTYUNDOBUFFER);
		_pEditView->restoreCurrentPos();
	}
	else
	{
		char msg[MAX_PATH + 100];
		strcpy(msg, "Can not open file \"");
		strcat(msg, fileName);
		strcat(msg, "\".");
		::MessageBox(_hSelf, msg, "ERR", MB_OK);
	}
}

void Notepad_plus::getMainClientRect(RECT &rc) const
{
    getClientRect(rc);
	rc.top += _rebarTop.getHeight();
	rc.bottom -= rc.top + _rebarBottom.getHeight() + _statusBar.getHeight();
}

void Notepad_plus::dockUserDlg()
{
    if (!_pMainSplitter)
    {
        _pMainSplitter = new SplitterContainer;
        _pMainSplitter->init(_hInst, _hSelf);

        Window *pWindow;
        if (_mainWindowStatus & TWO_VIEWS_MASK)
            pWindow = &_subSplitter;
        else
            pWindow = _pDocTab;

        _pMainSplitter->create(pWindow, ScintillaEditView::getUserDefineDlg(), 8, RIGHT_FIX, 45);
    }

    if (_mainWindowStatus & TWO_VIEWS_MASK)
        _pMainSplitter->setWin0(&_subSplitter);
    else 
        _pMainSplitter->setWin0(_pDocTab);

    _pMainSplitter->display();

    _mainWindowStatus |= DOCK_MASK;
    _pMainWindow = _pMainSplitter;

	::SendMessage(_hSelf, WM_SIZE, 0, 0);
}

void Notepad_plus::undockUserDlg()
{
    // a cause de surchargement de "display"
    ::ShowWindow(_pMainSplitter->getHSelf(), SW_HIDE);

    if (_mainWindowStatus & TWO_VIEWS_MASK)
        _pMainWindow = &_subSplitter;
    else
        _pMainWindow = _pDocTab;
    
    ::SendMessage(_hSelf, WM_SIZE, 0, 0);

    _mainWindowStatus &= ~DOCK_MASK;
    (ScintillaEditView::getUserDefineDlg())->display(); 
    //(_pEditView->getUserDefineDlg())->display();
}

void Notepad_plus::docGotoAnotherEditView(bool mode)
{
    if (!(_mainWindowStatus & TWO_VIEWS_MASK))
    {
        // if there's dock dialog, it means there's also a splitter container
        // we replace the right window by sub-spltter container that we just created
        if (_mainWindowStatus & DOCK_MASK)
        {
            _pMainSplitter->setWin0(&_subSplitter);
            _pMainWindow = _pMainSplitter;
        }
        else // otherwise the main window is the spltter container that we just created
            _pMainWindow = &_subSplitter;
        
        // resize the main window
        ::SendMessage(_hSelf, WM_SIZE, 0, 0);

        getNonCurrentEditView()->display();
        getNonCurrentDocTab()->display();

        _pMainWindow->display();

        // update the main window status
        _mainWindowStatus |= TWO_VIEWS_MASK;
    }

    // Bon, define the source view and the dest view
    // source view
    DocTabView *pSrcDocTab;
    ScintillaEditView *pSrcEditView;
    if (getCurrentView() == MAIN_VIEW)
    {
        // make dest view
        switchEditViewTo(SUB_VIEW);

        // make source view
        pSrcDocTab = &_mainDocTab;
        pSrcEditView = &_mainEditView;

    }
    else
    {
        // make dest view : _pDocTab & _pEditView
        switchEditViewTo(MAIN_VIEW);

        // make source view
        pSrcDocTab = &_subDocTab;
        pSrcEditView = &_subEditView;
    }

    // Maintenant, we begin to manipulate the source and the dest:
    // 1. Save the current position of the source view to transfer
    pSrcEditView->saveCurrentPos();

    // 2. Retrieve the current buffer from the source
    Buffer & buf = pSrcEditView->getCurrentBuffer();

    // 3. See if the file to transfer exist in the dest view
    //    if so, we don't transfer the file(buffer) 
    //    but activate the opened document in the dest view then beat it
    int i;
    if ( (i = _pDocTab->find(buf.getFileName())) != -1)
	{
		setTitleWith(_pDocTab->activate(i));
		_pDocTab->getFocus();
		return;
	}

    // 4. Transfer the file (buffer) into the dest view
    bool isNewDoc2Close = false;

    if ((_pEditView->getNbDoc() == 1) 
		&& Buffer::isUntitled(_pEditView->getCurrentTitle())
        && (!_pEditView->isCurrentDocDirty()) && (_pEditView->getCurrentDocLen() == 0))
    {
        isNewDoc2Close = true;
    }

    setTitleWith(_pDocTab->newDoc(buf));
    _pDocTab->updateCurrentTabItem(NULL);

    if (isNewDoc2Close)
        _pDocTab->closeDocAt(0);

    // 5. If it's the clone mode, we keep the document to transfer
    //    in the source view (do nothing). If it's the transfer mode
    //    we remove the file (buffer) from the source view

    if (mode != MODE_CLONE)
    {
        // Make focus to the source view
        switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);

        if (_pEditView->getNbDoc() == 1)
        {
			// close the current doc in the dest view
            _pDocTab->closeCurrentDoc();
			hideCurrentView();
        }
        else
        {
			// close the current doc in the dest view
            _pDocTab->closeCurrentDoc();

            // return to state where the focus is on dest view
            switchEditViewTo((getCurrentView() == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW);
        }
    }
	//printInt(getCurrentView());
	_linkTriggered = true;
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

int Notepad_plus::switchEditViewTo(int gid) 
{
	int oldView = getCurrentView();
    _pDocTab = (gid == MAIN_VIEW)?&_mainDocTab:&_subDocTab;
    _pEditView = (gid == MAIN_VIEW)?&_mainEditView:&_subEditView;
	_pEditView->beSwitched();
    _pEditView->getFocus();

    //checkDocState();
    setTitleWith(_pEditView->getCurrentTitle());

	::InvalidateRect(_mainDocTab.getHSelf(), NULL, TRUE);
	::InvalidateRect(_subDocTab.getHSelf(), NULL, TRUE);
	::SendMessage(_hSelf, NPPM_INTERNAL_DOCSWITCHIN, 0, 0);
	return oldView;
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
	enableConvertMenuItems((_pEditView->getCurrentBuffer()).getFormat());
	checkUnicodeMenuItems((_pEditView->getCurrentBuffer()).getUnicodeMode());

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

static bool isInList(string word, const vector<string> & wordArray)
{
	for (size_t i = 0 ; i < wordArray.size() ; i++)
		if (wordArray[i] == word)
			return true;
	return false;
};

void Notepad_plus::autoCompFromCurrentFile(bool autoInsert)
{
	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int startPos = int(_pEditView->execute(SCI_WORDSTARTPOSITION, curPos, true));

	if (curPos == startPos)
		return;

	const size_t bufSize = 256;
	size_t len = (curPos > startPos)?(curPos - startPos):(startPos - curPos);
	if (len >= bufSize)
		return;

	char beginChars[bufSize];

	_pEditView->getText(beginChars, startPos, curPos);

	string expr("\\<");
	expr += beginChars;
	expr += "[^ \\t.,;:\"()=<>'+!\\[\\]]*";

	//::MessageBox(NULL, expr.c_str(), "", MB_OK);

	int docLength = int(_pEditView->execute(SCI_GETLENGTH));

	int flags = SCFIND_WORDSTART | SCFIND_MATCHCASE | SCFIND_REGEXP | SCFIND_POSIX;

	_pEditView->execute(SCI_SETTARGETSTART, 0);
	_pEditView->execute(SCI_SETTARGETEND, docLength);
	_pEditView->execute(SCI_SETSEARCHFLAGS, flags);
	
	vector<string> wordArray;

	int posFind = int(_pEditView->execute(SCI_SEARCHINTARGET, expr.length(), (LPARAM)expr.c_str()));

	while (posFind != -1)
	{
		int wordStart = int(_pEditView->execute(SCI_GETTARGETSTART));
		int wordEnd = int(_pEditView->execute(SCI_GETTARGETEND));
		
		size_t foundTextLen = wordEnd - wordStart;

		if (foundTextLen < bufSize)
		{
			char w[bufSize];
			_pEditView->getText(w, wordStart, wordEnd);

			if (strcmp(w, beginChars))
				if (!isInList(w, wordArray))
					wordArray.push_back(w);
		}
		_pEditView->execute(SCI_SETTARGETSTART, wordEnd/*posFind + foundTextLen*/);
		_pEditView->execute(SCI_SETTARGETEND, docLength);
		posFind = int(_pEditView->execute(SCI_SEARCHINTARGET, expr.length(), (LPARAM)expr.c_str()));
	}
	if (wordArray.size() == 0) return;

	if (wordArray.size() == 1 && autoInsert) 
	{
		_pEditView->execute(SCI_SETTARGETSTART, startPos);
		_pEditView->execute(SCI_SETTARGETEND, curPos);
		_pEditView->execute(SCI_REPLACETARGETRE, wordArray[0].length(), (LPARAM)wordArray[0].c_str());
		
		_pEditView->execute(SCI_GOTOPOS, startPos + wordArray[0].length());
		return;
	}

	sort(wordArray.begin(), wordArray.end());

	string words("");

	for (size_t i = 0 ; i < wordArray.size() ; i++)
	{
		words += wordArray[i];
		if (i != wordArray.size()-1)
			words += " ";
	}

	_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM(' '));
	_pEditView->execute(SCI_AUTOCSETIGNORECASE, 3);
	_pEditView->execute(SCI_AUTOCSHOW, curPos - startPos, WPARAM(words.c_str()));

}

void Notepad_plus::showAutoComp()
{
	int curPos = int(_pEditView->execute(SCI_GETCURRENTPOS));
	int line = _pEditView->getCurrentLineNumber();
	int debutLinePos = int(_pEditView->execute(SCI_POSITIONFROMLINE, line ));
	int debutMotPos = curPos;


	char c = char(_pEditView->execute(SCI_GETCHARAT, debutMotPos-1));
	while ((debutMotPos>0)&&(debutMotPos>=debutLinePos)&&((isalnum(c))||(c=='_')))
	{
		debutMotPos--;
		c = char(_pEditView->execute(SCI_GETCHARAT, debutMotPos-1));
	}
	LangType langType = _pEditView->getCurrentDocType();
	if ((langType == L_RC) || (langType == L_HTML) || (langType == L_SQL))
	{
		int typeIndex = LANG_INDEX_INSTR;
		
		const char *pKeywords = (NppParameters::getInstance())->getWordList(langType, typeIndex);
		if (pKeywords)
		{
			_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM(' '));
			_pEditView->execute(SCI_AUTOCSETIGNORECASE, 3);
			_pEditView->execute(SCI_AUTOCSHOW, curPos-debutMotPos, WPARAM(pKeywords));
		}
	}
	else
	{
		char nppPath[MAX_PATH];
		strcpy(nppPath, _nppPath);
		PathRemoveFileSpec(nppPath);
		string fullFileName = nppPath;
		string fileName;
		getApiFileName(langType, fileName);
		fileName += ".api";
		fullFileName += "\\plugins\\APIs\\";
		fullFileName += fileName;

		FILE* f = fopen( fullFileName.c_str(), "r" );

		if (f)
		{
			fseek( f, 0, SEEK_END );
			size_t sz = ftell( f );
			fseek( f, 0, SEEK_SET );
			char* pData = new char[sz+1];
			size_t nbChar = fread(pData, 1, sz, f);
			pData[nbChar] = '\0';
			fclose( f );

			_pEditView->execute(SCI_AUTOCSETSEPARATOR, WPARAM('\n'));
			_pEditView->execute(SCI_AUTOCSETIGNORECASE, 3);
			_pEditView->execute(SCI_AUTOCSHOW, curPos-debutMotPos, WPARAM(pData));
			delete[] pData;
		}
	}
}

void Notepad_plus::changeMenuLang(string & pluginsTrans, string & windowTrans)
{
	if (!_nativeLang) return;

	TiXmlNode *mainMenu = _nativeLang->FirstChild("Menu");
	if (!mainMenu) return;

	mainMenu = mainMenu->FirstChild("Main");
	if (!mainMenu) return;

	TiXmlNode *entriesRoot = mainMenu->FirstChild("Entries");
	if (!entriesRoot) return;

	const char *idName = NULL;
	for (TiXmlNode *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		if (element->Attribute("id", &id))
		{
			const char *name = element->Attribute("name");
			::ModifyMenu(_mainMenuHandle, id, MF_BYPOSITION, 0, name);
		}
		else if (idName = element->Attribute("idName"))
		{
			const char *name = element->Attribute("name");
			if (!strcmp(idName, "Plugins"))
			{
				pluginsTrans = name;
			}
			else if (!strcmp(idName, "Window"))
			{
				windowTrans = name;
			}
		}
	}

	TiXmlNode *menuCommandsRoot = mainMenu->FirstChild("Commands");

	for (TiXmlNode *childNode = menuCommandsRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		::ModifyMenu(_mainMenuHandle, id, MF_BYCOMMAND, id, name);
	}

	TiXmlNode *subEntriesRoot = mainMenu->FirstChild("SubEntries");

	for (TiXmlNode *childNode = subEntriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int x, y;
		element->Attribute("posX", &x);
		element->Attribute("posY", &y);
		const char *name = element->Attribute("name");
		::ModifyMenu(::GetSubMenu(_mainMenuHandle, x), y, MF_BYPOSITION, 0, name);
	}
	::DrawMenuBar(_hSelf);
}

void Notepad_plus::changeConfigLang()
{
	if (!_nativeLang) return;

	TiXmlNode *styleConfDlgNode = _nativeLang->FirstChild("Dialog");
	if (!styleConfDlgNode) return;	
	
	styleConfDlgNode = styleConfDlgNode->FirstChild("StyleConfig");
	if (!styleConfDlgNode) return;

	HWND hDlg = _configStyleDlg.getHSelf();
	// Set Title
	const char *titre = (styleConfDlgNode->ToElement())->Attribute("title");
	if ((titre && titre[0]) && hDlg)
		::SetWindowText(hDlg, titre);

	for (TiXmlNode *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
				::SetWindowText(hItem, name);
		}
	}
	hDlg = _configStyleDlg.getHSelf();
	styleConfDlgNode = styleConfDlgNode->FirstChild("SubDialog");
	
	for (TiXmlNode *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
				::SetWindowText(hItem, name);
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
				::SetWindowText(hItem, translatedText[i]);
		}
	}
}

void Notepad_plus::changeUserDefineLang()
{
	if (!_nativeLang) return;

	TiXmlNode *userDefineDlgNode = _nativeLang->FirstChild("Dialog");
	if (!userDefineDlgNode) return;	
	
	userDefineDlgNode = userDefineDlgNode->FirstChild("UserDefine");
	if (!userDefineDlgNode) return;

	UserDefineDialog *userDefineDlg = _pEditView->getUserDefineDlg();

	HWND hDlg = userDefineDlg->getHSelf();
	// Set Title
	const char *titre = (userDefineDlgNode->ToElement())->Attribute("title");
	if (titre && titre[0])
		::SetWindowText(hDlg, titre);

	// pour ses propres controls 	
	const int nbControl = 9;
	const char *translatedText[nbControl];
	for (int i = 0 ; i < nbControl ; i++)
		translatedText[i] = NULL;

	for (TiXmlNode *childNode = userDefineDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		
		if (sentinel && (name && name[0]))
		{
			if (id > 30)
			{
				HWND hItem = ::GetDlgItem(hDlg, id);
				if (hItem)
					::SetWindowText(hItem, name);
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

	//int **idArrays[nbDlg] = {(int **)folderID, (int **)keywordsID, (int **)commentID, (int **)operatorID};

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
		TiXmlNode *node = userDefineDlgNode->FirstChild(nodeNameArray[i]);
		
		if (node) 
		{
			// Set Title
			titre = (node->ToElement())->Attribute("title");
			if (titre &&titre[0])
				userDefineDlg->setTabName(i, titre);

			for (TiXmlNode *childNode = node->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElement *element = childNode->ToElement();
				int id;
				const char *sentinel = element->Attribute("id", &id);
				const char *name = element->Attribute("name");
				if (sentinel && (name && name[0]))
				{
					HWND hItem = ::GetDlgItem(hDlgArrary[i], id);
					if (hItem)
						::SetWindowText(hItem, name);
				}
			}
		}
	}
}

void Notepad_plus::changePrefereceDlgLang() 
{
	changeDlgLang(_preference.getHSelf(), "Preference");

	char title[64];
	
	changeDlgLang(_preference._barsDlg.getHSelf(), "Global", title);
	if (*title)
		_preference._ctrlTab.renameTab("Global", title);

	changeDlgLang(_preference._marginsDlg.getHSelf(), "Scintillas", title);
	if (*title)
		_preference._ctrlTab.renameTab("Scintillas", title);

	changeDlgLang(_preference._defaultNewDocDlg.getHSelf(), "NewDoc", title);
	if (*title)
		_preference._ctrlTab.renameTab("NewDoc", title);

	changeDlgLang(_preference._fileAssocDlg.getHSelf(), "FileAssoc", title);
	if (*title)
		_preference._ctrlTab.renameTab("FileAssoc", title);

	changeDlgLang(_preference._langMenuDlg.getHSelf(), "LangMenu", title);
	if (*title)
		_preference._ctrlTab.renameTab("LangMenu", title);

	changeDlgLang(_preference._printSettingsDlg.getHSelf(), "Print1", title);
	if (*title)
		_preference._ctrlTab.renameTab("Print1", title);

	changeDlgLang(_preference._printSettings2Dlg.getHSelf(), "Print2", title);
	if (*title)
		_preference._ctrlTab.renameTab("Print2", title);

	changeDlgLang(_preference._settingsDlg.getHSelf(), "MISC", title);
	if (*title)
		_preference._ctrlTab.renameTab("MISC", title);

	changeDlgLang(_preference._backupDlg.getHSelf(), "Backup", title);
	if (*title)
		_preference._ctrlTab.renameTab("Backup", title);

}

void Notepad_plus::changeShortcutLang()
{
	if (!_nativeLang) return;

	NppParameters * pNppParam = NppParameters::getInstance();
	vector<CommandShortcut> & mainshortcuts = pNppParam->getUserShortcuts();
	vector<ScintillaKeyMap> & scinshortcuts = pNppParam->getScintillaKeyList();
	int mainSize = (int)mainshortcuts.size();
	int scinSize = (int)scinshortcuts.size();

	TiXmlNode *shortcuts = _nativeLang->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Main");
	if (!shortcuts) return;

	TiXmlNode *entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNode *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int index, id;
		if (element->Attribute("index", &index) && element->Attribute("id", &id))
		{
			if (index > -1 && index < mainSize) { //valid index only
				const char *name = element->Attribute("name");
				CommandShortcut & csc = mainshortcuts[index];
				if (csc.getID() == id) {
					csc.setName(name);
				}
			}
		}
	}

	//Scintilla
	shortcuts = _nativeLang->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Scintilla");
	if (!shortcuts) return;

	entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNode *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < scinSize) { //valid index only
				const char *name = element->Attribute("name");
				ScintillaKeyMap & skm = scinshortcuts[index];
				skm.setName(name);
			}
		}
	}

}

void Notepad_plus::changeShortcutmapperLang(ShortcutMapper * sm)
{
	if (!_nativeLang) return;

	TiXmlNode *shortcuts = _nativeLang->FirstChild("Dialog");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("ShortcutMapper");
	if (!shortcuts) return;

	for (TiXmlNode *childNode = shortcuts->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < 5) { //valid index only
				const char *name = element->Attribute("name");
				sm->translateTab(index, name);
			}
		}
	}
}


TiXmlNode * searchDlgNode(TiXmlNode *node, const char *dlgTagName)
{
	TiXmlNode *dlgNode = node->FirstChild(dlgTagName);
	if (dlgNode) return dlgNode;
	for (TiXmlNode *childNode = node->FirstChildElement();
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

	if (!_nativeLang) return false;

	TiXmlNode *dlgNode = _nativeLang->FirstChild("Dialog");
	if (!dlgNode) return false;

	dlgNode = searchDlgNode(dlgNode, dlgTagName);
	if (!dlgNode) return false;

	// Set Title
	const char *titre = (dlgNode->ToElement())->Attribute("title");
	if ((titre && titre[0]) && hDlg)
	{
		::SetWindowText(hDlg, titre);
		if (title)
			strcpy(title, titre);
	}

	// Set the text of child control
	for (TiXmlNode *childNode = dlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElement *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
				::SetWindowText(hItem, name);
		}
	}
	return true;
}

static string extractSymbol(char prefix, const char *str2extract)
{
	bool found = false;
	char extracted[128] = "";

	for (size_t i = 0, j = 0 ; i < strlen(str2extract) ; i++)
	{
		if (found)
		{
			if (!str2extract[i] || str2extract[i] == ' ')
			{
				extracted[j] = '\0';
				return string(extracted);
			}
			extracted[j++] = str2extract[i];

		}
		else
		{
			if (!str2extract[i])
				return "";

			if (str2extract[i] == prefix)
				found = true;
		}
	}
	return  string(extracted);
};

bool Notepad_plus::doBlockComment(comment_mode currCommentMode)
{
	const char *commentLineSybol;
	string symbol;

	Buffer & buf = _pEditView->getCurrentBuffer();
	if (buf._lang == L_USER)
	{
		UserLangContainer & userLangContainer = NppParameters::getInstance()->getULCFromName(buf._userLangExt);
		//::MessageBox(NULL, userLangContainer._keywordLists[4], "User", MB_OK);
		symbol = extractSymbol('0', userLangContainer._keywordLists[4]);
		commentLineSybol = symbol.c_str();
	}
	else
		commentLineSybol = buf.getCommentLineSymbol();


	if ((!commentLineSybol) || (!commentLineSybol[0]))
		return false;

    string comment(commentLineSybol);
    comment += " ";
    string long_comment = comment;
    
    char linebuf[1000];
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
        if ((lineEnd - lineIndent) >= static_cast<int>(sizeof(linebuf)))        // Avoid buffer size problems
                continue;
        /*if (props.GetInt(comment_at_line_start.c_str())) {
                GetRange(wEditor, lineIndent, lineEnd, linebuf);
        } else*/
        {
            lineIndent = _pEditView->execute(SCI_GETLINEINDENTPOSITION, i);
            _pEditView->getText(linebuf, lineIndent, lineEnd);
        }
        // empty lines are not commented
        if (strlen(linebuf) < 1)
			continue;
   		if (currCommentMode != cm_comment)
		{
			if (memcmp(linebuf, comment.c_str(), comment_length - 1) == 0)
			{
				if (memcmp(linebuf, long_comment.c_str(), comment_length) == 0)
				{
					// removing comment with space after it
					_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + comment_length);
					_pEditView->execute(SCI_REPLACESEL, 0, (WPARAM)"");
					if (i == selStartLine) // is this the first selected line?
						selectionStart -= comment_length;
					selectionEnd -= comment_length; // every iteration
					continue;
				}
				else
				{
					// removing comment _without_ space
					_pEditView->execute(SCI_SETSEL, lineIndent, lineIndent + comment_length - 1);
					_pEditView->execute(SCI_REPLACESEL, 0, (WPARAM)"");
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
			_pEditView->execute(SCI_INSERTTEXT, lineIndent, (WPARAM)long_comment.c_str());
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
	const char *commentStart;
	const char *commentEnd;

	string symbolStart;
	string symbolEnd;

	Buffer & buf = _pEditView->getCurrentBuffer();
	if (buf._lang == L_USER)
	{
		UserLangContainer & userLangContainer = NppParameters::getInstance()->getULCFromName(buf._userLangExt);
		symbolStart = extractSymbol('1', userLangContainer._keywordLists[4]);
		commentStart = symbolStart.c_str();
		symbolEnd = extractSymbol('2', userLangContainer._keywordLists[4]);
		commentEnd = symbolEnd.c_str();
	}
	else
	{
		commentStart = _pEditView->getCurrentBuffer().getCommentStart();
		commentEnd = _pEditView->getCurrentBuffer().getCommentEnd();
	}

	if ((!commentStart) || (!commentStart[0]))
		return false;
	if ((!commentEnd) || (!commentEnd[0]))
		return false;

	string start_comment(commentStart);
	string end_comment(commentEnd);
	string white_space(" ");

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

		char linebuf[1000];
		_pEditView->getText(linebuf, lineIndent, lineEnd);
	    
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
	_pEditView->execute(SCI_INSERTTEXT, selectionStart, (WPARAM)start_comment.c_str());
	selectionEnd += start_comment_length;
	selectionStart += start_comment_length;
	_pEditView->execute(SCI_INSERTTEXT, selectionEnd, (WPARAM)end_comment.c_str());
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

bool Notepad_plus::switchToFile(const char *fileName)
{
	if (!fileName) return false;
	int i = - 1;
	int iView;

	if ((i = _mainDocTab.find(fileName)) != -1)
	{
		iView = MAIN_VIEW;
	}
	else if ((i = _subDocTab.find(fileName)) != -1)
	{
		iView = SUB_VIEW;
	}

	if (i != -1)
	{
		switchEditViewTo(iView);
		setTitleWith(_pDocTab->activate(i));
		_pEditView->getFocus();
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
	int iView = getCurrentView();
	ScintillaEditView & currentView = (iView == MAIN_VIEW)?_mainEditView:_subEditView;
	ScintillaEditView & nonCurrentView = (iView == MAIN_VIEW)?_subEditView:_mainEditView;
	int nonCurrentiView = (iView == MAIN_VIEW)?SUB_VIEW:MAIN_VIEW;

	size_t currentNbDoc = currentView.getNbDoc();
	size_t nonCurrentNbDoc;
	
	tli->_currentIndex = 0;

	if (iView == MAIN_VIEW)
	{
		nonCurrentNbDoc = _subDocTab.isVisible()?_subEditView.getNbDoc():0;
	}
	else
	{
		nonCurrentNbDoc = _mainDocTab.isVisible()?_mainEditView.getNbDoc():0;
	}

	for (size_t i = 0 ; i < currentNbDoc ; i++)
	{
		Buffer & b = currentView.getBufferAt(i);
		int status = b.isReadOnly()?tb_ro:(b.isDirty()?tb_unsaved:tb_saved);
		tli->_tlfsLst.push_back(TaskLstFnStatus(iView,i,b._fullPathName, status));
	}
	for (size_t i = 0 ; i < nonCurrentNbDoc ; i++)
	{
		Buffer & b = nonCurrentView.getBufferAt(i);
		int status = b.isReadOnly()?tb_ro:(b.isDirty()?tb_unsaved:tb_saved);
		tli->_tlfsLst.push_back(TaskLstFnStatus(nonCurrentiView,i,b._fullPathName, status));
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
			/* Note: lParam is -1 to prevent endless loops of calls */
			::SendMessage(_dockingManager.getHSelf(), WM_NCACTIVATE, wParam, (LPARAM)-1);
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}
		case WM_CREATE:
		{
			char * acTop_xpm[] = {
				"14 14 4 1", //0
				" 	c #FFFFFF", //1
				".	c #000000", //2
				"+	c #A400B7", //3
				"@	c #DE25F4", //4
				"++++++++++++++",
				" +@@@@@@@@@@. ",
				"  +@@@@@@@@.  ",
				"   +@@@@@@.   ",
				"    +@@@@.    ",
				"     +@@.     ",
				"      +.      ",
				"              ",
				"              ",
				"      @@      ",
				"      @@      ",
				"              ",
				"      @@      ",
				"      @@      "};

			char * acBottom_xpm[] = {
				"14 14 4 1", //0
				" 	c #FFFFFF", //1
				".	c #000000", //2
				"+	c #A400B7", //3
				"@	c #DE25F4", //4
				"      @@      ",
				"      @@      ",
				"              ",
				"      @@      ",
				"      @@      ",
				"              ",
				"              ",
				"      +.      ",
				"     +@@.     ",
				"    +@@@@.    ",
				"   +@@@@@@.   ",
				"  +@@@@@@@@.  ",
				" +@@@@@@@@@@. ",
				".............."};
			
			pNppParam->setFontList(hwnd);
			NppGUI & nppGUI = (NppGUI &)pNppParam->getNppGUI();

			// Menu
			_mainMenuHandle = ::GetMenu(_hSelf);

            _pDocTab = &_mainDocTab;
            _pEditView = &_mainEditView;

            const ScintillaViewParams & svp1 = pNppParam->getSVP(SCIV_PRIMARY);
			const ScintillaViewParams & svp2 = pNppParam->getSVP(SCIV_SECOND);

			_mainEditView.init(_hInst, hwnd);
			_subEditView.init(_hInst, hwnd);
            
			_mainEditView.display();

			_mainEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESBEGIN, (LPARAM)acTop_xpm);   
			_mainEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESEND, (LPARAM)acBottom_xpm);   
			_subEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESBEGIN, (LPARAM)acTop_xpm);
			_subEditView.execute(SCI_MARKERDEFINEPIXMAP, MARK_HIDELINESEND, (LPARAM)acBottom_xpm);

			_invisibleEditView.init(_hInst, hwnd);
			_invisibleEditView.execute(SCI_SETUNDOCOLLECTION);
			_invisibleEditView.execute(SCI_EMPTYUNDOBUFFER);
			_invisibleEditView.attatchDefaultDoc(0);

			// Configuration of 2 scintilla views
            _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp1._lineNumberMarginShow);
			_subEditView.showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp2._lineNumberMarginShow);
            _mainEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp1._bookMarkMarginShow);
			_subEditView.showMargin(ScintillaEditView::_SC_MARGE_SYBOLE, svp2._bookMarkMarginShow);

            _mainEditView.showIndentGuideLine(svp1._indentGuideLineShow);
            _subEditView.showIndentGuideLine(svp2._indentGuideLineShow);
			
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

			if (pNppParam->hasCustomContextMenu())
			{
				_mainEditView.execute(SCI_USEPOPUP, FALSE);
				_subEditView.execute(SCI_USEPOPUP, FALSE);
			}

			_zoomOriginalValue = _pEditView->execute(SCI_GETZOOM);
			_mainEditView.execute(SCI_SETZOOM, svp1._zoom);
			_subEditView.execute(SCI_SETZOOM, svp2._zoom);

			int tabBarStatus = nppGUI._tabStatus;
			_toReduceTabBar = ((tabBarStatus & TAB_REDUCE) != 0);
			_docTabIconList.create(_toReduceTabBar?13:20, _hInst, docTabIconIDs, sizeof(docTabIconIDs)/sizeof(int));

			_subDocTab.init(_hInst, hwnd, &_subEditView, &_docTabIconList);
            
			const char * str = _mainDocTab.init(_hInst, hwnd, &_mainEditView, &_docTabIconList);
			setTitleWith(str);
            
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
			//TabBarPlus::setMultiLine((tabBarStatus & TAB_MULTILINE) != 0);

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

			//dynamicCheckMenuAndTB();
			_mainEditView.defineDocType(L_TXT);

			if (nppGUI._isMinimizedToTray)
				_pTrayIco = new trayIconControler(_hSelf, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)), "");

			checkSyncState();
			
			// Plugin Manager
			NppData nppData;
			nppData._nppHandle = _hSelf;
			nppData._scintillaMainHandle = _mainEditView.getHSelf();
			nppData._scintillaSecondHandle = _subEditView.getHSelf();

			_scintillaCtrls4Plugins.init(_hInst, hwnd);

			_pluginsManager.init(nppData);
			_pluginsManager.loadPlugins();
			const char *appDataNpp = pNppParam->getAppDataNppDir();
			if (appDataNpp[0])
				_pluginsManager.loadPlugins(appDataNpp);

			// Menu
			string pluginsTrans, windowTrans;
			changeMenuLang(pluginsTrans, windowTrans);

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
				//::MessageBox(NULL, "pas de updater", "", MB_OK);
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
				char buffer[100];

				int x;
				for(x = 0; (x == 0 || strcmp(externalLangContainer._name, buffer) > 0) && x < numLangs; x++)
				{
					::GetMenuString(hLangMenu, x, buffer, sizeof(buffer), MF_BYPOSITION);
				}

				::InsertMenu(hLangMenu, x-1, MF_BYPOSITION, IDM_LANG_EXTERNAL + i, externalLangContainer._name);
			}

			if (nppGUI._excludedLangList.size() > 0)
			{
				for (size_t i = 0 ; i < nppGUI._excludedLangList.size() ; i++)
				{
					int cmdID = pNppParam->langTypeToCommandID(nppGUI._excludedLangList[i]._langType);
					char itemName[256];
					::GetMenuString(hLangMenu, cmdID, itemName, sizeof(itemName), MF_BYCOMMAND);
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

			//Plugin menu
			_pluginsManager.setMenu(_mainMenuHandle, pluginsTrans.c_str());

			//Windows menu
			_windowsMenu.init(_hInst, _mainMenuHandle, windowTrans.c_str());

			//Add recent files
			HMENU hFileMenu = ::GetSubMenu(_mainMenuHandle, MENUINDEX_FILE);
			int nbLRFile = pNppParam->getNbLRFile();
			int pos = 16;

			_lastRecentFileList.initMenu(hFileMenu, IDM_FILEMENU_LASTONE + 1, pos);
			for (int i = 0 ; i < nbLRFile ; i++)
			{
				string * stdStr = pNppParam->getLRFile(i);
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

			//The menu is loaded, add in all the accelerators

			// Update context menu strings
			vector<MenuItemUnit> & tmp = pNppParam->getContextMenuItems();
			size_t len = tmp.size();
			char menuName[nameLenMax];
			*menuName = 0;
			size_t j, stlen;
			for (size_t i = 0 ; i < len ; i++)
			{
				if (tmp[i]._itemName == "") {
					::GetMenuString(_mainMenuHandle, tmp[i]._cmdID, menuName, 64, MF_BYCOMMAND);
					stlen = strlen(menuName);
					j = 0;
					for(size_t k = 0; k < stlen; k++) {
						if (menuName[k] == '\t') {
							menuName[k] = 0;
							break;
						} else if (menuName[k] == '&') {
							//skip
						} else {
							menuName[j] = menuName[k];
							j++;
						}
					}
					menuName[j] = 0;
					tmp[i]._itemName = menuName;
				}
			}


			//Input all the menu item names into shortcut list
			//This will automatically do all translations, since menu translation has been done already
			vector<CommandShortcut> & shortcuts = pNppParam->getUserShortcuts();
			len = shortcuts.size();
			int readI;
			for(size_t i = 0; i < len; i++) {
				CommandShortcut & csc = shortcuts[i];
				if (!csc.getName()[0]) {	//no predefined name, get name from menu and use that
					if (::GetMenuString(_mainMenuHandle, csc.getID(), menuName, 64, MF_BYCOMMAND)) {
						readI = 0;
						while(menuName[readI] != 0) 
						{
							if (menuName[readI] == '\t') 
							{
								menuName[readI] = 0;
								break;
							}
							readI++;
						}
					}
				csc.setName(menuName);
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
            char name[256];
			strcpy(name, (char *)lParam);
			_mainEditView.removeUserLang(name);
			_subEditView.removeUserLang(name);
			return TRUE;
		}

        case WM_RENAME_USERLANG:
		{
            char oldName[256];
			char newName[256];
			strcpy(oldName, (char *)lParam);
			strcpy(newName, (char *)wParam);
			_mainEditView.renameUserLang(oldName, newName);
			_subEditView.renameUserLang(oldName, newName);
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
			bool isRecursive = lParam == TRUE;
			findInFiles(isRecursive);
			return TRUE;
		}

		case NPPM_LAUNCHFINDINFILESDLG :
		{
			const int strSize = 64;
			char str[strSize];

			bool isFirstTime = !_findReplaceDlg.isCreated();
			_findReplaceDlg.doDialog(FIND_DLG, _isRTL);

			_pEditView->getSelectedText(str, strSize);
			_findReplaceDlg.setSearchText(str);
			if (isFirstTime)
				changeDlgLang(_findReplaceDlg.getHSelf(), "Find");
			_findReplaceDlg.launchFindInFilesDlg();
			
			const char *dir = NULL;
			char currentDir[MAX_PATH];
			string fltr;

			if (wParam)
				dir = (const char *)wParam;
			else
			{
				::GetCurrentDirectory(MAX_PATH, currentDir);
				dir = currentDir;
			}

			if (lParam)
			{
				fltr = (const char *)lParam;
			}
			else
			{
				LangType lt = _pEditView->getCurrentDocType();

				const char *ext = NppParameters::getInstance()->getLangExtFromLangType(lt);
				if (ext && ext[0])
				{
					string filtres = "";
					vector<string> vStr;
					cutString(ext, vStr);
					for (size_t i = 0 ; i < vStr.size() ; i++)
					{
							filtres += "*.";
							filtres += vStr[i] + " ";
					}
					fltr = filtres;
				}
				else
					fltr = "*.*";
			}
			_findReplaceDlg.setFindInFilesDirFilter(dir, fltr.c_str());
			return TRUE;
		}

		case WM_DOOPEN:
		{
			doOpen((const char *)lParam);
		}
		break;

		case NPPM_RELOADFILE:
		{
			doReload((const char *)lParam, wParam != 0);
		}
		break;

		case NPPM_SWITCHTOFILE :
		{
			return switchToFile((const char *)lParam);
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
			//_pMainWindow->reSizeTo(rc);

			//mkPosIncFindDlg();
			result = TRUE;
		}
		break;

		case WM_MOVE:
		{
			//redraw();
			//mkPosIncFindDlg();
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
			//const DWORD LASTBYTEMASK = 0x000000FF;
            COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT *)lParam;

			switch (pCopyData->dwData)
			{
				case COPYDATA_PARAMS :
				{
					pNppParam->setCmdlineParam(*((CmdLineParams *)pCopyData->lpData));
					break;
				}

				case COPYDATA_FILENAMES :
				{
					CmdLineParams & cmdLineParams = pNppParam->getCmdLineParams();
 					LangType lt = cmdLineParams._langType;//LangType(pCopyData->dwData & LASTBYTEMASK);
					int ln =  cmdLineParams._line2go;
					int currentDocIndex = _pEditView->getCurrentDocIndex();
					int currentView = getCurrentView();

					FileNameStringSplitter fnss((char *)pCopyData->lpData);
					char *pFn = NULL;
					for (int i = 0 ; i < fnss.size() ; i++)
					{
						pFn = (char *)fnss.getFileName(i);
						doOpen((const char *)pFn, cmdLineParams._isReadOnly);
						if (lt != L_TXT)
						{
							_pEditView->setCurrentDocType(lt);
							setLangStatus(lt);
							checkLangsMenu(-1);
						}
						_pEditView->execute(SCI_GOTOLINE, ln-1);
					}

					if (_isSaving == true) 
					{
						switchEditViewTo(currentView);
						setTitleWith(_pDocTab->activate(currentDocIndex));
						return true;
					}
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

		case NPPM_MENUCOMMAND :
			command(lParam);
			return TRUE;

		case NPPM_GETFULLCURRENTPATH :
		case NPPM_GETCURRENTDIRECTORY :
		case NPPM_GETFILENAME :
		case NPPM_GETNAMEPART :
		case NPPM_GETEXTPART :
		{
			char str[MAX_PATH];
			// par defaut : NPPM_GETCURRENTDIRECTORY
			char *fileStr = strcpy(str, _pEditView->getCurrentTitle());

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

			// For the compability reason, if wParam is 0, then we assume the size of string buffer (lParam) is large enough.
			// otherwise we check if the string buffer size is enough for the string to copy.
			if (wParam != 0)
			{
				if (strlen(fileStr) >= wParam)
				{
					::MessageBox(_hSelf, "Allocated buffer size is not enough to copy the string.", "NPPM error", MB_OK);
					return FALSE;
				}
			}

			strcpy((char *)lParam, fileStr);
			return TRUE;
		}

		case NPPM_GETCURRENTWORD :
		{
			const int strSize = MAX_PATH;
			char str[strSize];

			_pEditView->getSelectedText((char *)str, strSize);
			// For the compability reason, if wParam is 0, then we assume the size of string buffer (lParam) is large enough.
			// otherwise we check if the string buffer size is enough for the string to copy.
			if (wParam != 0)
			{
				if (strlen(str) >= wParam)	//buffer too small
				{
					::MessageBox(_hSelf, "Allocated buffer size is not enough to copy the string.", "NPPM_GETCURRENTWORD error", MB_OK);
					return FALSE;
				}
				else						//buffer large enough, perform safe copy
				{
					lstrcpyn((char *)lParam, str, wParam);
					return TRUE;
				}
			}

			strcpy((char *)lParam, str);
			return TRUE;
		}

		case NPPM_GETNPPDIRECTORY :
		{
			const int strSize = MAX_PATH;
			char str[strSize];

			::GetModuleFileName(NULL, str, strSize);
			PathRemoveFileSpec(str);

			// For the compability reason, if wParam is 0, then we assume the size of string buffer (lParam) is large enough.
			// otherwise we check if the string buffer size is enough for the string to copy.
			if (wParam != 0)
			{
				if (strlen(str) >= wParam)
				{
					::MessageBox(_hSelf, "Allocated buffer size is not enough to copy the string.", "NPPM_GETNPPDIRECTORY error", MB_OK);
					return FALSE;
				}
			}

			strcpy((char *)lParam, str);
			return TRUE;
		}

		case NPPM_GETCURRENTSCINTILLA :
		{
			*((int *)lParam) = (_pEditView == &_mainEditView)?0:1;
			return TRUE;
		}

		case NPPM_GETCURRENTLANGTYPE :
		{
			*((LangType *)lParam) = _pEditView->getCurrentDocType();
			return TRUE;
		}

		case NPPM_SETCURRENTLANGTYPE :
		{
			_pEditView->setCurrentDocType((LangType)lParam);
			return TRUE;
		}

		case NPPM_GETNBOPENFILES :
		{
			int nbDocPrimary = _mainEditView.getNbDoc();
			int nbDocSecond = _subEditView.getNbDoc();
			if (lParam == ALL_OPEN_FILES)
				return nbDocPrimary + nbDocSecond;
			else if (lParam == PRIMARY_VIEW)
				return  nbDocPrimary;
			else if (lParam == SECOND_VIEW)
				return  nbDocSecond;
		}

		case NPPM_GETOPENFILENAMESPRIMARY :
		{
			if (!wParam) return 0;

			char **fileNames = (char **)wParam;
			size_t nbFileNames = lParam;
			size_t i = 0;

			for ( ; i < nbFileNames ; i++)
			{
				strcpy(fileNames[i], _mainEditView.getBufferAt(i).getFileName());
			} 
			return i;
		}

		case NPPM_GETOPENFILENAMESSECOND :
		{
			if (!wParam) return 0;

			char **fileNames = (char **)wParam;
			size_t nbFileNames = lParam;
			size_t i = 0;

			for ( ; i < nbFileNames ; i++)
			{
				strcpy(fileNames[i], _subEditView.getBufferAt(i).getFileName());
			} 
			return i;
		}

		case NPPM_GETOPENFILENAMES :
		{
			if (!wParam) return 0;

			char **fileNames = (char **)wParam;
			int nbFileNames = lParam;

			int j = 0;
			for (size_t i = 0 ; i < _mainEditView.getNbDoc() && j < nbFileNames ; i++)
			{
				strcpy(fileNames[j++], _mainEditView.getBufferAt(i).getFileName());
			}
			for (size_t i = 0 ; i < _subEditView.getNbDoc() && j < nbFileNames ; i++)
			{
				strcpy(fileNames[j++], _subEditView.getBufferAt(i).getFileName());
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
				std::sort(tli->_tlfsLst.begin(),tli->_tlfsLst.end(),SortTaskListPred(_mainEditView,_subEditView));
			}
			else
			{
				for(int idx = 0; idx < (int)tli->_tlfsLst.size(); ++idx)
				{
					if(tli->_tlfsLst[idx]._iView == getCurrentView() &&
						tli->_tlfsLst[idx]._docIndex == getCurrentEditView()->getCurrentDocIndex())
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
					int nbDoc = _mainDocTab.isVisible()?_mainEditView.getNbDoc():0;
					nbDoc += _subDocTab.isVisible()?_subEditView.getNbDoc():0;
					if (nbDoc > 1)
						activateNextDoc((GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD)?dirDown:dirUp);
					_linkTriggered = true;
			}
			return ::DefWindowProc(hwnd, Message, wParam, lParam);
		}

		case NPPM_GETNBSESSIONFILES :
		{
			const char *sessionFileName = (const char *)lParam;
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
			const char *sessionFileName = (const char *)lParam;
			char **sessionFileArray = (char **)wParam;

			if ((!sessionFileName) || (sessionFileName[0] == '\0')) return FALSE;

			Session session2Load;
			if (pNppParam->loadSession(session2Load, sessionFileName))
			{
				size_t i = 0;
				for ( ; i < session2Load.nbMainFiles() ; )
				{
					const char *pFn = session2Load._mainViewFiles[i]._fileName.c_str();
					strcpy(sessionFileArray[i++], pFn);
				}

				for (size_t j = 0 ; j < session2Load.nbSubFiles() ; j++)
				{
					const char *pFn = session2Load._subViewFiles[j]._fileName.c_str();
// comment to remove
/*
					// make sure that the same file is not cloned in another view
					bool foundInMainView = false;
					for (size_t k = 0 ; k < session2Load.nbMainFiles() ; k++)
					{
						const char *pFnMain = session2Load._mainViewFiles[k]._fileName.c_str();
						if (strcmp(pFn, pFnMain) == 0)
						{
							foundInMainView = true;
							break;
						}
					}
					if (!foundInMainView)
*/
						strcpy(sessionFileArray[i++], pFn);
				}
				return TRUE;
			}
			return FALSE;
		}

		case NPPM_DECODESCI:
		{
			/* convert to ASCII */
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
			

			/* get text of current scintilla */
			length = pSci->execute(SCI_GETTEXTLENGTH, 0, 0) + 1;
			buffer = new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

			/* convert here */       
			UniMode unicodeMode = pSci->getCurrentBuffer().getUnicodeMode();
			UnicodeConvertor.setEncoding(unicodeMode);
			length = UnicodeConvertor.convert(buffer, length-1);

			/* set text in target */
			pSci->execute(SCI_CLEARALL);
			pSci->execute(SCI_ADDTEXT, length, (LPARAM)UnicodeConvertor.getNewBuf());
			pSci->execute(SCI_EMPTYUNDOBUFFER);

			pSci->execute(SCI_SETCODEPAGE);

			/* set cursor position */
			pSci->execute(SCI_GOTOPOS);

			/* clean buffer */
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
			buffer = (char*)new char[length];
			pSci->execute(SCI_GETTEXT, length, (LPARAM)buffer);

			length = UnicodeConvertor.convert(buffer, length-1);

			// set text in target
			pSci->execute(SCI_CLEARALL);
			pSci->execute(SCI_ADDTEXT, length, (LPARAM)UnicodeConvertor.getNewBuf());

			pSci->execute(SCI_EMPTYUNDOBUFFER);

			// set cursor position
			pSci->execute(SCI_GOTOPOS);

			// clean buffer
			delete [] buffer;

			// set new encoding if BOM was changed by other programms
			UniMode um = UnicodeConvertor.getEncoding();
			(pSci->getCurrentBuffer()).setUnicodeMode(um);
			checkUnicodeMenuItems(um);

			// Override the code page if Unicode
			if (um != uni8Bit)
				_pEditView->execute(SCI_SETCODEPAGE, SC_CP_UTF8);

			return um;
		}

		case NPPM_ACTIVATEDOC :
		case NPPM_TRIGGERTABBARCONTEXTMENU:
		{
			// similar to NPPM_ACTIVEDOC
			int whichView = ((wParam != MAIN_VIEW) && (wParam != SUB_VIEW))?getCurrentView():wParam;
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
			char verStr[16] = VERSION_VALUE;
			char mainVerStr[16];
			char auxVerStr[16];
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
				mainVer = atoi(mainVerStr);
			if (auxVerStr)
				auxVer = atoi(auxVerStr);

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
				if (!_subDocTab.isVisible())
					return -1;
				return _subEditView.getCurrentDocIndex();
			}
			else //MAIN_VIEW
			{
				if (!_mainDocTab.isVisible())
					return -1;
				return _mainEditView.getCurrentDocIndex();
			}
		}

		case NPPM_SETSTATUSBAR :
		{
			char *str2set = (char *)lParam;
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
			fileLoadSession((const char *)lParam);
			return TRUE;
		}

		case NPPM_SAVECURRENTSESSION :
		{
			return (LRESULT)fileSaveSession(0, NULL, (const char *)lParam);
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
			//changeMenuShortcut(lParam, (const char *)wParam);
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

		case NPPM_INTERNAL_SCINTILLAKEYMODIFIED :
		{
			return TRUE;
		}

		case NPPM_INTERNAL_PLUGINCMDLIST_MODIFIED :
		{
			return TRUE;
		}

		case NPPM_INTERNAL_DOCSWITCHOFF :
		{
			removeHideLinesBookmarks();
			return TRUE;
		}

		case NPPM_INTERNAL_DOCSWITCHIN :
		{
			_hideLinesMarks.empty();

			checkDocState();
			dynamicCheckMenuAndTB();
			setLangStatus(_pEditView->getCurrentDocType());
			updateStatusBar();
			reloadOnSwitchBack();
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
			_mainEditView.defineDocType(_mainEditView.getCurrentDocType());
			_subEditView.defineDocType(_subEditView.getCurrentDocType());
			_mainEditView.performGlobalStyles();
			_subEditView.performGlobalStyles();
			return TRUE;
		}

		//case WM_ENDSESSION:
		case WM_QUERYENDSESSION:
		case WM_CLOSE:
		{
			if (_isfullScreen)
				fullScreenToggle();

			const NppGUI & nppgui = pNppParam->getNppGUI();
			
			if (_configStyleDlg.isCreated() && ::IsWindowVisible(_configStyleDlg.getHSelf()))
				_configStyleDlg.restoreGlobalOverrideValues();

			Session currentSession;
			if (nppgui._rememberLastSession)
				getCurrentOpenedFiles(currentSession);

			if (fileCloseAll())
			{
				SCNotification scnN;
				scnN.nmhdr.code = NPPN_SHUTDOWN;
				scnN.nmhdr.hwndFrom = _hSelf;
				scnN.nmhdr.idFrom = 0;
				_pluginsManager.notify(&scnN);

				_lastRecentFileList.saveLRFL();
				saveScintillaParams(SCIV_PRIMARY);
				saveScintillaParams(SCIV_SECOND);
				saveGUIParams();
				saveUserDefineLangs();
				saveShortcuts();
				if (nppgui._rememberLastSession)
					saveSession(currentSession);
			
				::DestroyWindow(hwnd);
			}
			return TRUE;
		}

		case WM_DESTROY:
		{	
			killAllChildren();	
			::PostQuitMessage(0);
			return TRUE;
		}

		case WM_SYSCOMMAND:
		{
			NppGUI & nppgui = (NppGUI &)(pNppParam->getNppGUI());
			if ((nppgui._isMinimizedToTray) && (wParam == SC_MINIMIZE))
			{
				if (!_pTrayIco)
					_pTrayIco = new trayIconControler(_hSelf, IDI_M30ICON, IDC_MINIMIZED_TRAY, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_M30ICON)), "");

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
			_dockingManager.showDockableDlg((char*)lParam, SW_SHOW);
			return TRUE;
		}

		case NPPM_DMMGETPLUGINHWNDBYNAME : //(const char *windowName, const char *moduleName)
		{
			if (!lParam) return NULL;

			char *moduleName = (char *)lParam;
			char *windowName = (char *)wParam;
			vector<DockingCont *> dockContainer = _dockingManager.getContainerInfo();
			for (size_t i = 0 ; i < dockContainer.size() ; i++)
			{
				vector<tTbData *> tbData = dockContainer[i]->getDataOfAllTb();
				for (size_t j = 0 ; j < tbData.size() ; j++)
				{
					if (stricmp(moduleName, tbData[j]->pszModuleName) == 0)
					{
						if (!windowName)
							return (LRESULT)tbData[j]->hClient;
						else if (stricmp(windowName, tbData[j]->pszName) == 0)
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
			_pEditView->setCurrentDocState(true);
			_pDocTab->updateCurrentTabItem();
			checkDocState();
			synchronise();
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

			const char *pluginsConfigDirPrefix = pNppParam->getAppDataNppDir();
			
			if (!pluginsConfigDirPrefix[0])
				pluginsConfigDirPrefix = pNppParam->getNppPath();

			const char *secondPart = "plugins\\Config";
			
			size_t len = (size_t)wParam;
			if (len < strlen(pluginsConfigDirPrefix) + strlen(secondPart))
				return FALSE;

			char *pluginsConfigDir = (char *)lParam;			
			strcpy(pluginsConfigDir, pluginsConfigDirPrefix);

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
			bool oldVal = _mainDocTab.setHideTabBarStatus(hide);
			_subDocTab.setHideTabBarStatus(hide);
			::SendMessage(_hSelf, WM_SIZE, 0, 0);

			NppGUI & nppGUI = (NppGUI &)((NppParameters::getInstance())->getNppGUI());
			if (hide)
				nppGUI._tabStatus |= TAB_HIDE;
			else
				nppGUI._tabStatus &= ~TAB_HIDE;

			return oldVal;
		}

		case NPPM_ISTABBARHIDE :
		{
			return _mainDocTab.getHideTabBarStatus();
		}

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

		case NPPM_INTERNAL_ISFOCUSEDTAB :
		{
			ScintillaEditView *cv = getCurrentEditView();
			HWND pMainTB = (_mainDocTab.getView() == cv)?_mainDocTab.getHSelf():_subDocTab.getHSelf();
			return (HWND)lParam == pMainTB;
		}

		case NPPM_INTERNAL_GETMENU :
		{
			return (LRESULT)_mainMenuHandle;
		}
		
		case WM_INITMENUPOPUP:
		{
			_windowsMenu.initPopupMenu((HMENU)wParam, _pEditView);
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
						int origPos = _pEditView->getCurrentDocIndex();
						for (int i=0, n=nmdlg->nItems; i<n; ++i) {
							activateDoc(nmdlg->Items[i]);
							fileSave();
						}
						activateDoc(origPos);
						nmdlg->processed = TRUE;
					}
					break;
				case WDT_CLOSE:
					{
						for (int i=0, n=nmdlg->nItems; i<n; ++i) {
							UINT pos = nmdlg->Items[i];
							activateDoc(pos);
							if (!fileClose()) 
								break;
							for (int j=i+1; j<n; ++j)
								if (nmdlg->Items[j] > pos) 
									--nmdlg->Items[j];
							nmdlg->Items[i] = 0xFFFFFFFF; // indicate file was closed
						}
						nmdlg->processed = TRUE;
					}
					break;
				case WDT_SORT:
					_pEditView->arrangeBuffers(nmdlg->nItems, nmdlg->Items);
					nmdlg->processed = TRUE;
					for (int i = _pEditView->getNbDoc()-1 ; i >= 0  ; --i)
					{
						Buffer & docBuf = _pEditView->getBufferAt(i);
						_pDocTab->updateTabItem(i, PathFindFileName(docBuf.getFileName()));
					}
					activateDoc(nmdlg->curSel);
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
		::SetWindowLong(hwnd, GWL_USERDATA, (LONG)pM30ide);

		return TRUE;
	}

    default :
    {
      return ((Notepad_plus *)::GetWindowLong(hwnd, GWL_USERDATA))->runProc(hwnd, Message, wParam, lParam);
    }
  }
}

void Notepad_plus::fullScreenToggle()
{
	HMONITOR currentMonitor;	//Handle to monitor where fullscreen should go
	MONITORINFO mi;				//Info of that monitor
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

		//Caution, this will not work on windows 95, so probably add some checking of some sorts like Unicode checks, IF 95 were to be supported
		currentMonitor = MonitorFromWindow(_hSelf, MONITOR_DEFAULTTONEAREST);	//should always be valid monitor handle
		mi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(currentMonitor, &mi) != FALSE)
		{
			fullscreenArea = mi.rcMonitor;
			fullscreenArea.right -= fullscreenArea.left;
			fullscreenArea.bottom -= fullscreenArea.top;
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

		if (!stricmp(pddi._name, dockData.pszModuleName) && (pddi._internalID == dockData.dlgID))
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
	_pEditView->saveCurrentPos();
	session._activeView = getCurrentView();
	int currentDocIndex = session._activeMainIndex = _mainEditView.getCurrentDocIndex();
	//int currentDocIndex = _mainEditView.getCurrentDocIndex();

	for (size_t i = 0 ; i < _mainEditView.getNbDoc() ; i++)
	{
		const Buffer & buf = _mainEditView.getBufferAt((size_t)i);
		if (!Buffer::isUntitled(buf._fullPathName) && PathFileExists(buf._fullPathName))
		{
			string	languageName	= getLangFromMenu( buf );
			const char *langName	= languageName.c_str();

			sessionFileInfo sfi(buf._fullPathName, langName, buf._pos);

			_mainEditView.activateDocAt(i);
			int maxLine = _mainEditView.execute(SCI_GETLINECOUNT);
			for (int j = 0 ; j < maxLine ; j++)
			{
				if ((_mainEditView.execute(SCI_MARKERGET, j)&(1 << MARK_BOOKMARK)) != 0)
				{
					sfi.marks.push_back(j);
				}
			}
			session._mainViewFiles.push_back(sfi);
		}
	}
	_mainEditView.activateDocAt(currentDocIndex);

	session._activeSubIndex = _subEditView.getCurrentDocIndex();
	currentDocIndex = _subEditView.getCurrentDocIndex();
	for (size_t i = 0 ; i < _subEditView.getNbDoc() ; i++)
	{
		const Buffer & buf = _subEditView.getBufferAt((size_t)i);
		if (PathFileExists(buf._fullPathName))
		{
			string	languageName	= getLangFromMenu( buf );
			const char *langName	= languageName.c_str();

			sessionFileInfo sfi(buf._fullPathName, langName, buf._pos);

			_subEditView.activateDocAt(i);
			int maxLine = _subEditView.execute(SCI_GETLINECOUNT);
			for (int j = 0 ; j < maxLine ; j++)
			{
				if ((_subEditView.execute(SCI_MARKERGET, j)&(1 << MARK_BOOKMARK)) != 0)
				{
					sfi.marks.push_back(j);
				}
			}
			session._subViewFiles.push_back(sfi);
		}
	}
	_subEditView.activateDocAt(currentDocIndex);
}

bool Notepad_plus::fileLoadSession(const char *fn)
{
	bool result = false;
	const char *sessionFileName = NULL;
	if (fn == NULL)
	{
		FileDialog fDlg(_hSelf, _hInst);
		fDlg.setExtFilter("All types", ".*", NULL);
		const char *ext = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();
		string sessionExt = "";
		if (*ext != '\0')
		{
			if (*ext != '.') 
				sessionExt += ".";
			sessionExt += ext;
			fDlg.setExtFilter("Session file", sessionExt.c_str(), NULL);
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


const char * Notepad_plus::fileSaveSession(size_t nbFile, char ** fileNames, const char *sessionFile2save)
{
	if (sessionFile2save)
	{
		Session currentSession;
		if ((nbFile) && (!fileNames))
		{

			for (size_t i = 0 ; i < nbFile ; i++)
			{
				if (PathFileExists(fileNames[i]))
					currentSession._mainViewFiles.push_back(string(fileNames[i]));
			}
		}
		else
			getCurrentOpenedFiles(currentSession);

		(NppParameters::getInstance())->writeSession(currentSession, sessionFile2save);
		return sessionFile2save;
	}
	return NULL;
}

const char * Notepad_plus::fileSaveSession(size_t nbFile, char ** fileNames)
{
	const char *sessionFileName = NULL;
	
	FileDialog fDlg(_hSelf, _hInst);
	const char *ext = NppParameters::getInstance()->getNppGUI()._definedSessionExt.c_str();

	fDlg.setExtFilter("All types", ".*", NULL);
	string sessionExt = "";
	if (*ext != '\0')
	{
		if (*ext != '.') 
			sessionExt += ".";
		sessionExt += ext;
		fDlg.setExtFilter("Session file", sessionExt.c_str(), NULL);
	}
	sessionFileName = fDlg.doSaveDlg();

	return fileSaveSession(nbFile, fileNames, sessionFileName);
}


bool Notepad_plus::str2Cliboard(const char *str2cpy)
{
	if (!str2cpy)
		return false;
		
	if (!::OpenClipboard(_hSelf)) 
        return false; 
		
    ::EmptyClipboard();
	
	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, strlen(str2cpy) + 1);
	
	if (hglbCopy == NULL) 
	{ 
		::CloseClipboard(); 
		return false; 
	} 

	// Lock the handle and copy the text to the buffer. 
	char *pStr = (char *)::GlobalLock(hglbCopy);
	strcpy(pStr, str2cpy);
	::GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
	::SetClipboardData(CF_TEXT, hglbCopy);
	::CloseClipboard();
	return true;
}


void Notepad_plus::markSelectedText()
{
	//Get selection
	CharacterRange range = _pEditView->getSelection();
	//Dont mark if the selection has not changed.
	if (range.cpMin == _prevSelectedRange.cpMin && range.cpMax == _prevSelectedRange.cpMax)
	{
		return;
	}
	_prevSelectedRange = range;

	//Clear marks
	LangType lt = _pEditView->getCurrentDocType();
	if (lt == L_TXT)
	_pEditView->defineDocType(L_CPP);
	_pEditView->defineDocType(lt);

	//If nothing selected, dont mark anything
	if (range.cpMin == range.cpMax)
	{
		return;
	}

	char text2Find[MAX_PATH];
	_pEditView->getSelectedText(text2Find, sizeof(text2Find), false);	//do not expand selection (false)

	FindOption op;
	op._isWholeWord = false;
	_findReplaceDlg.markAll2(text2Find);
}


typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

winVer getWindowsVersion()
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
			return WV_UNKNOWN;
	}

	pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if(pGNSI != NULL)
		pGNSI(&si);
	else
		GetSystemInfo(&si);

   switch (osvi.dwPlatformId)
   {
		case VER_PLATFORM_WIN32_NT:
		{
			if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
			{
				return WV_VISTA;
			}

			if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
			{
				if (osvi.wProductType == VER_NT_WORKSTATION &&
					   si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
				{
					return WV_XPX64;
				}
				return WV_S2003;
			}

			if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
				return WV_XP;

			if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
				return WV_W2K;

			if ( osvi.dwMajorVersion <= 4 )
				return WV_NT;
		}
		break;

		// Test for the Windows Me/98/95.
		case VER_PLATFORM_WIN32_WINDOWS:
		{
			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
			{
				return WV_95;
			} 

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
			{
				return WV_98;
			} 

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			{
				return WV_ME;
			}
		}
		break;

      case VER_PLATFORM_WIN32s:
		return WV_WIN32S;
      
	  default :
		return WV_UNKNOWN;
   }
   return WV_UNKNOWN; 
}


