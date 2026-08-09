// This file is part of Notepad++ project
// Copyright (C)2025 Don HO <don.h@free.fr>

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

#include <windows.h>

// ATTENTION : It's a part of interface header, so don't include the others header here

// styles for containers
#define	CAPTION_TOP				TRUE
#define	CAPTION_BOTTOM			FALSE

//   defines for docking manager
#define	CONT_LEFT		0
#define	CONT_RIGHT		1
#define	CONT_TOP		2
#define	CONT_BOTTOM		3
#define	DOCKCONT_MAX	4

// mask params for plugins of internal dialogs
#define DWS_ICONTAB			0x00000001			// Icon for tabs are available
#define DWS_ICONBAR			0x00000002			// Icon for icon bar are available (currently not supported)
#define DWS_ADDINFO			0x00000004			// Additional information are in use
#define DWS_USEOWNDARKMODE	0x00000008			// Use plugin's own dark mode
#define DWS_PARAMSALL		(DWS_ICONTAB|DWS_ICONBAR|DWS_ADDINFO)

// default docking values for first call of plugin
#define DWS_DF_CONT_LEFT	(CONT_LEFT	<< 28)	// default docking on left
#define DWS_DF_CONT_RIGHT	(CONT_RIGHT	<< 28)	// default docking on right
#define DWS_DF_CONT_TOP		(CONT_TOP	<< 28)	// default docking on top
#define DWS_DF_CONT_BOTTOM	(CONT_BOTTOM << 28)	// default docking on bottom
#define DWS_DF_FLOATING		0x80000000			// default state is floating


typedef struct DockedWidgetData {            // DockedWidget data (old name: tTbData)
	HWND hClient = nullptr;                  // client Window Handle
	const wchar_t* pszName = nullptr;        // name of plugin (shown in window)
	int dlgID = 0;                           // a funcItem provides the function pointer to start a dialog. Please parse here these ID

	// user modifications
	UINT uMask = 0;                          // mask params: look to above defines
	HICON hIconTab = nullptr;                // icon for tabs
	const wchar_t* pszAddInfo = nullptr;     // for plugin to display additional information

	// internal data, do not use !!! (rcFloat, iPrevCont only)
	RECT rcFloat = {}; // floating position
	int iPrevCont = 0; // stores the privious container (toggling between float and dock)

	// REQUIRED: must be set by the plugin, NOT internal data.
	// This must be the plugin's own DLL file name (e.g. L"NppPluginDemo.dll"),
	// including the ".dll" extension - not the panel's display title, and not an arbitrary identifier string.
	// Notepad++'s Docking Manager uses this file name to locate and reload the plugin's DLL, and to re-open/re-dock this panel on the next startup.
	// The value is persisted verbatim as the "pluginName" attribute of the <PluginDlg> element in the user's config.xml, e.g.:
	// <PluginDlg pluginName="NppPluginDemo.dll" id="16" curr="1" prev="-1" isVisible="yes" />
	// If this is left null/empty or set incorrectly, the panel will fail to restore its docked position/visibility across restarts.
	const wchar_t* pszModuleName = nullptr;

} DockedWidgetData, tTbData;


struct tDockMgr {
	HWND hWnd = nullptr;                  // the docking manager wnd
	RECT rcRegion[DOCKCONT_MAX] = {{}};   // position of docked dialogs
};


#define	HIT_TEST_THICKNESS		20
#define SPLITTER_WIDTH			4

