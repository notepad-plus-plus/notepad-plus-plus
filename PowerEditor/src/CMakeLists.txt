# @file:<dictaolib-root>/CMakeLists.txt

# TODO
# - Move defs XX_INCLUDE_DIR, XX_LIBRARIES, XX_LIBRARY_DIRS to the subdirectories
#   scripts. I could not find a way to export variables defined in a subscript
#   so as a workaround, I define the variables myself in the top script. This is
#   not optimal.
#

cmake_minimum_required(VERSION 3.4)

PROJECT(Notepad++)

SET(projIncludDir	../../scintilla/include/
					../../lexilla/include/
					./
					./WinControls/AboutDlg/
					./WinControls/
					./WinControls/ImageListSet/
					./WinControls/OpenSaveFileDialog/
					./WinControls/SplitterContainer/
					./WinControls/StaticDialog/
					./WinControls/TabBar/
					./WinControls/ToolBar/
					./MISC/Process/
					./ScintillaComponent/
					./MISC/
					./MISC/SysMsg/
					./WinControls/StatusBar/
					./WinControls/StaticDialog/RunDlg/
					./tinyxml/
					./WinControls/ColourPicker/
					./Win32Explr/
					./MISC/RegExt/
					./WinControls/TrayIcon/
					./WinControls/shortcut/
					./WinControls/Grid/
					./WinControls/ContextMenu/
					./MISC/PluginsManager/
					./WinControls/Preference/
					./WinControls/WindowsDlg/
					./WinControls/TaskList/
					./WinControls/DockingWnd/
					./WinControls/TreeView/
					./WinControls/ToolTip/
					./MISC/Exception/
					./MISC/Common/
					./tinyxml/tinyXmlA/
					./WinControls/AnsiCharPanel/
					./WinControls/ClipboardHistory/
					./WinControls/FindCharsInRange/
					./WinControls/VerticalFileSwitcher/
					./WinControls/ProjectPanel/
					./WinControls/DocumentMap/
					./WinControls/FunctionList/
					./uchardet/
					./WinControls/FileBrowser/
					./WinControls/ReadDirectoryChanges/
					./MISC/md5/
					./MISC/sha2/
					./WinControls/PluginsAdmin/
					./json/)



SET(src_files
			./DarkMode/DarkMode.cpp
			./MISC/Common/verifySignedfile.cpp
			./MISC/md5/md5Dlgs.cpp
			./MISC/sha2/sha-256.cpp
			./NppDarkMode.cpp
			./MISC/Process/Processus.cpp
			./WinControls/AboutDlg/AboutDlg.cpp
			./WinControls/AnsiCharPanel/ansiCharPanel.cpp
			./ScintillaComponent/AutoCompletion.cpp
			./WinControls/AnsiCharPanel/asciiListView.cpp
			./WinControls/FileBrowser/fileBrowser.cpp
			./WinControls/Grid/BabyGrid.cpp
			./WinControls/Grid/BabyGridWrapper.cpp
			./ScintillaComponent/Buffer.cpp
			./uchardet/CharDistribution.cpp
			./WinControls/ClipboardHistory/clipboardHistoryPanel.cpp
			./WinControls/ColourPicker/ColourPicker.cpp
			./WinControls/ColourPicker/ColourPopup.cpp
			./ScintillaComponent/columnEditor.cpp
			./MISC/Common/Common.cpp
			./MISC/Common/FileInterface.cpp
			./WinControls/ContextMenu/ContextMenu.cpp
			./WinControls/OpenSaveFileDialog/CustomFileDialog.cpp
			./WinControls/PluginsAdmin/pluginsAdmin.cpp
			./WinControls/ReadDirectoryChanges/ReadDirectoryChanges.cpp
			./WinControls/ReadDirectoryChanges/ReadDirectoryChangesPrivate.cpp
			./WinControls/ReadDirectoryChanges/ReadFileChanges.cpp
			./WinControls/TabBar/ControlsTab.cpp
			./WinControls/DockingWnd/DockingCont.cpp
			./WinControls/DockingWnd/DockingManager.cpp
			./WinControls/DockingWnd/DockingSplitter.cpp
			./ScintillaComponent/DocTabView.cpp
			./WinControls/DocumentMap/documentMap.cpp
			./WinControls/DocumentMap/documentSnapshot.cpp
			./EncodingMapper.cpp
			./WinControls/FindCharsInRange/FindCharsInRange.cpp
			./ScintillaComponent/FindReplaceDlg.cpp
			./ScintillaComponent/FunctionCallTip.cpp
			./WinControls/FunctionList/functionListPanel.cpp
			./WinControls/FunctionList/functionParser.cpp
			./ScintillaComponent/GoToLineDlg.cpp
			./WinControls/DockingWnd/Gripper.cpp
			./MISC/PluginsManager/IDAllocator.cpp
			./WinControls/ImageListSet/ImageListSet.cpp
			./uchardet/JpCntx.cpp
			./uchardet/LangBulgarianModel.cpp
			./uchardet/LangCyrillicModel.cpp
			./uchardet/LangGreekModel.cpp
			./uchardet/LangHebrewModel.cpp
			./uchardet/LangHungarianModel.cpp
			./uchardet/LangThaiModel.cpp
			./lastRecentFileList.cpp
			./lesDlgs.cpp
			./WinControls/AnsiCharPanel/ListView.cpp
			./localization.cpp
			./MISC/Exception/MiniDumper.cpp
			./Notepad_plus.cpp
			./Notepad_plus_Window.cpp
			./NppBigSwitch.cpp
			./NppCommands.cpp
			./NppIO.cpp
			./NppNotification.cpp
			./uchardet/nsBig5Prober.cpp
			./uchardet/nsCharSetProber.cpp
			./uchardet/nsEscCharsetProber.cpp
			./uchardet/nsEscSM.cpp
			./uchardet/nsEUCJPProber.cpp
			./uchardet/nsEUCKRProber.cpp
			./uchardet/nsEUCTWProber.cpp
			./uchardet/nsGB2312Prober.cpp
			./uchardet/nsHebrewProber.cpp
			./uchardet/nsLatin1Prober.cpp
			./uchardet/nsMBCSGroupProber.cpp
			./uchardet/nsMBCSSM.cpp
			./uchardet/nsSBCharSetProber.cpp
			./uchardet/nsSBCSGroupProber.cpp
			./uchardet/nsSJISProber.cpp
			./uchardet/nsUniversalDetector.cpp
			./uchardet/nsUTF8Prober.cpp
			./Parameters.cpp
			./Misc/PluginsManager/PluginsManager.cpp
			./WinControls/Preference/preferenceDlg.cpp
			./ScintillaComponent/Printer.cpp
			./WinControls/ProjectPanel/ProjectPanel.cpp
			./MISC/RegExt/regExtDlg.cpp
			./WinControls/StaticDialog/RunDlg/RunDlg.cpp
			./WinControls/shortcut/RunMacroDlg.cpp
			./ScintillaComponent/ScintillaCtrls.cpp
			./ScintillaComponent/ScintillaEditView.cpp
			./WinControls/shortcut/shortcut.cpp
			./WinControls/Grid/ShortcutMapper.cpp
			./WinControls/TreeView/TreeView.cpp
			./WinControls/WindowsDlg/SizeableDlg.cpp
			./ScintillaComponent/SmartHighlighter.cpp
			./WinControls/SplitterContainer/Splitter.cpp
			./WinControls/SplitterContainer/SplitterContainer.cpp
			./WinControls/StaticDialog/StaticDialog.cpp
			./WinControls/StatusBar/StatusBar.cpp
			./WinControls/TabBar/TabBar.cpp
			./WinControls/TaskList/TaskList.cpp
			./WinControls/TaskList/TaskListDlg.cpp
			./TinyXml/tinystr.cpp
			./TinyXml/tinyXmlA/tinystrA.cpp
			./TinyXml/tinyxml.cpp
			./TinyXml/tinyXmlA/tinyxmlA.cpp
			./TinyXml/tinyxmlerror.cpp
			./TinyXml/tinyXmlA/tinyxmlerrorA.cpp
			./TinyXml/tinyxmlparser.cpp
			./TinyXml/tinyXmlA/tinyxmlparserA.cpp
			./WinControls/ToolBar/ToolBar.cpp
			./WinControls/ToolTip/ToolTip.cpp
			./WinControls/TrayIcon/trayIconControler.cpp
			./uchardet/uchardet.cpp
			./WinControls/AboutDlg/URLCtrl.cpp
			./ScintillaComponent/UserDefineDialog.cpp
			./Utf8_16.cpp
			./WinControls/VerticalFileSwitcher/VerticalFileSwitcher.cpp
			./WinControls/VerticalFileSwitcher/VerticalFileSwitcherListView.cpp
			./MISC/Exception/Win32Exception.cpp
			./WinControls/WindowsDlg/WindowsDlg.cpp
			./winmain.cpp
			./WinControls/WindowsDlg/WinMgr.cpp
			./WinControls/WindowsDlg/WinRect.cpp
			./WinControls/ColourPicker/WordStyleDlg.cpp
			./ScintillaComponent/xmlMatchedTagsHighlighter.cpp
			./dpiAware.manifest
			./notepad++.exe.manifest
)

SET(include_files
			./clipboardFormats.h
			./DarkMode/DarkMode.h
			./DarkMode/IatHook.h
			./DarkMode/UAHMenuBar.h
			./dpiManager.h
			./keys.h
			./localizationString.h
			./MISC/Common/Sorters.h
			./MISC/Common/verifySignedfile.h
			./MISC/md5/md5.h
			./MISC/md5/md5Dlgs.h
			./MISC/md5/md5Dlgs_rc.h
			./MISC/sha2/sha-256.h
			./NppDarkMode.h
			./ScintillaComponent/columnEditor_rc.h
			./ScintillaComponent/FindReplaceDlg_rc.h
			./ScintillaComponent/ScintillaRef.h
			./Utf8.h
			./WinControls/AboutDlg/AboutDlg.h
			./WinControls/AnsiCharPanel/ansiCharPanel.h
			./ScintillaComponent/AutoCompletion.h
			./WinControls/AnsiCharPanel/ansiCharPanel_rc.h
			./WinControls/AnsiCharPanel/asciiListView.h
			./WinControls/ColourPicker/ColourPopupResource.h
			./WinControls/ColourPicker/WordStyleDlgRes.h
			./WinControls/DocumentMap/documentMap_rc.h
			./WinControls/DocumentMap/documentSnapshot.h
			./WinControls/DocumentMap/documentSnapshot_rc.h
			./WinControls/FindCharsInRange/findCharsInRange_rc.h
			./WinControls/Grid/BabyGrid.h
			./WinControls/Grid/BabyGridWrapper.h
			./ScintillaComponent/Buffer.h
			./uchardet/CharDistribution.h
			./WinControls/ClipboardHistory/clipboardHistoryPanel.h
			./WinControls/ClipboardHistory/clipboardHistoryPanel_rc.h
			./ScintillaComponent/colors.h
			./WinControls/ColourPicker/ColourPicker.h
			./WinControls/ColourPicker/ColourPopup.h
			./ScintillaComponent/columnEditor.h
			./MISC/Common/Common.h
			./MISC/Common/FileInterface.h
			./WinControls/ContextMenu/ContextMenu.h
			./WinControls/OpenSaveFileDialog/CustomFileDialog.h
			./WinControls/PluginsAdmin/pluginsAdmin.h
			./WinControls/PluginsAdmin/pluginsAdminRes.h
			./WinControls/Preference/preference_rc.h
			./WinControls/ReadDirectoryChanges/targetver.h
			./WinControls/shortcut/shortcutRc.h
			./WinControls/StaticDialog/RunDlg/RunDlg_rc.h
			./WinControls/TabBar/ControlsTab.h
			./WinControls/DockingWnd/Docking.h
			./WinControls/DockingWnd/DockingCont.h
			./WinControls/DockingWnd/DockingDlgInterface.h
			./WinControls/DockingWnd/DockingManager.h
			./WinControls/DockingWnd/dockingResource.h
			./WinControls/DockingWnd/DockingSplitter.h
			./ScintillaComponent/DocTabView.h
			./WinControls/DocumentMap/documentMap.h
			./WinControls/DocumentMap/documentSnapshot.h
			./EncodingMapper.h
			./MISC/FileNameStringSplitter.h
			./WinControls/FindCharsInRange/FindCharsInRange.h
			./ScintillaComponent/FindReplaceDlg.h
			./ScintillaComponent/FunctionCallTip.h
			./WinControls/FunctionList/functionListPanel.h
			./WinControls/FunctionList/functionListPanel_rc.h
			./WinControls/FunctionList/functionParser.h
			./ScintillaComponent/GoToLineDlg.h
			./WinControls/DockingWnd/Gripper.h
			./MISC/PluginsManager/IDAllocator.h
			./WinControls/ImageListSet/ImageListSet.h
			./uchardet/JpCntx.h
			./lastRecentFileList.h
			./lesDlgs.h
			./WinControls/AnsiCharPanel/ListView.h
			./localization.h
			./menuCmdID.h
			./MISC/Exception/MiniDumper.h
			./Notepad_plus.h
			./MISC/PluginsManager/Notepad_plus_msgs.h
			./Notepad_plus_Window.h
			./uchardet/nsBig5Prober.h
			./uchardet/nsCharSetProber.h
			./uchardet/nsCodingStateMachine.h
			./uchardet/nscore.h
			./uchardet/nsEscCharsetProber.h
			./uchardet/nsEUCJPProber.h
			./uchardet/nsEUCKRProber.h
			./uchardet/nsEUCTWProber.h
			./uchardet/nsGB2312Prober.h
			./uchardet/nsHebrewProber.h
			./uchardet/nsLatin1Prober.h
			./uchardet/nsMBCSGroupProber.h
			./uchardet/nsPkgInt.h
			./uchardet/nsSBCharSetProber.h
			./uchardet/nsSBCSGroupProber.h
			./uchardet/nsSJISProber.h
			./uchardet/nsUniversalDetector.h
			./uchardet/nsUTF8Prober.h
			./Parameters.h
			./MISC/PluginsManager/PluginInterface.h
			./MISC/PluginsManager/PluginsManager.h
			./WinControls/Preference/preferenceDlg.h
			./ScintillaComponent/Printer.h
			./uchardet/prmem.h
			./WinControls/ProjectPanel/ProjectPanel.h
			./WinControls/ProjectPanel/ProjectPanel_rc.h
			./MISC/RegExt/regExtDlg.h
			./MISC/RegExt/regExtDlgRc.h
			./resource.h
			./WinControls/StaticDialog/RunDlg/RunDlg.h
			./WinControls/shortcut/RunMacroDlg.h
			./WinControls/shortcut/RunMacroDlg_rc.h
			./ScintillaComponent/ScintillaCtrls.h
			./ScintillaComponent/ScintillaEditView.h
			./WinControls/shortcut/shortcut.h
			./WinControls/Grid/ShortcutMapper.h
			./WinControls/Grid/ShortcutMapper_rc.h
			./WinControls/TreeView/TreeView.h
			./WinControls/WindowsDlg/SizeableDlg.h
			./ScintillaComponent/SmartHighlighter.h
			./WinControls/SplitterContainer/Splitter.h
			./WinControls/SplitterContainer/SplitterContainer.h
			./WinControls/StaticDialog/StaticDialog.h
			./WinControls/StatusBar/StatusBar.h
			./WinControls/TabBar/TabBar.h
			./WinControls/TaskList/TaskList.h
			./WinControls/TaskList/TaskListDlg.h
			./WinControls/TaskList/TaskListDlg_rc.h
			./TinyXml/tinystr.h
			./TinyXml/tinyXmlA/tinystrA.h
			./TinyXml/tinyxml.h
			./TinyXml/tinyXmlA/tinyxmlA.h
			./WinControls/ToolBar/ToolBar.h
			./WinControls/ToolTip/ToolTip.h
			./WinControls/TrayIcon/trayIconControler.h
			./uchardet/uchardet.h
			./WinControls/AboutDlg/URLCtrl.h
			./ScintillaComponent/UserDefineDialog.h
			./ScintillaComponent/UserDefineLangReference.h
			./ScintillaComponent/UserDefineResource.h
			./Utf8_16.h
			./WinControls/VerticalFileSwitcher/VerticalFileSwitcher.h
			./WinControls/VerticalFileSwitcher/VerticalFileSwitcher_rc.h
			./WinControls/VerticalFileSwitcher/VerticalFileSwitcherListView.h
			./MISC/Exception/Win32Exception.h
			./WinControls/Window.h
			./WinControls/WindowsDlg/WindowsDlg.h
			./WinControls/WindowsDlg/WindowsDlgRc.h
			./WinControls/WindowsDlg/WinMgr.h
			./WinControls/ColourPicker/WordStyleDlg.h
			./ScintillaComponent/xmlMatchedTagsHighlighter.h
			./rgba_icons.h
)


SET(rcFiles
			./MISC/md5/md5Dlgs.rc
			./WinControls/AnsiCharPanel/ansiCharPanel.rc
			./WinControls/ClipboardHistory/clipboardHistoryPanel.rc
			./WinControls/ColourPicker/ColourPopup.rc
			./ScintillaComponent/columnEditor.rc
			./WinControls/DockingWnd/DockingGUIWidget.rc
			./WinControls/DocumentMap/documentMap.rc
			./WinControls/DocumentMap/documentSnapshot.rc
			./WinControls/FileBrowser/fileBrowser.rc
			./WinControls/FindCharsInRange/findCharsInRange.rc
			./ScintillaComponent/FindReplaceDlg.rc
			./WinControls/FunctionList/functionListPanel.rc
			./Notepad_plus.rc
			./WinControls/PluginsAdmin/pluginsAdmin.rc
			./WinControls/Preference/preference.rc
			./WinControls/ProjectPanel/ProjectPanel.rc
			./MISC/RegExt/regExtDlg.rc
			./WinControls/StaticDialog/RunDlg/RunDlg.rc
			./WinControls/shortcut/RunMacroDlg.rc
			./WinControls/shortcut/shortcut.rc
			./WinControls/Grid/ShortcutMapper.rc
			./WinControls/TaskList/TaskListDlg.rc
			./ScintillaComponent/UserDefineDialog.rc
			./WinControls/VerticalFileSwitcher/VerticalFileSwitcher.rc
			./WinControls/WindowsDlg/WindowsDlg.rc
			./WinControls/ColourPicker/WordStyleDlg.rc
)

IF (WIN32)
	SET(option WIN32)
	SET(win32_LIBRARIES comctl32 shlwapi dbghelp version crypt32 wintrust sensapi wininet imm32 msimg32)
if ( MSVC )
	#do not use for mingw builds
	SET(CMAKE_CXX_FLAGS "/EHa /MP /W4")
	SET(defs -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600 -D_USE_64BIT_TIME_T -DTIXML_USE_STL -DTIXMLA_USE_STL -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS )
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else ( MSVC )
	# For possible MinGW compilation
	SET(CMAKE_CXX_FLAGS "-include../gcc/include/various.h -std=c++17 -fpermissive -municode")
	SET(defs -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600 -D_USE_64BIT_TIME_T -DTIXML_USE_STL -DTIXMLA_USE_STL )
endif ( MSVC )
ENDIF (WIN32)

ADD_DEFINITIONS(${defs})

INCLUDE_DIRECTORIES(${projIncludDir})

find_library(SCINTILLA_STATIC_LIBRARY
	NAMES libscintilla
	HINTS "../../scintilla/bin"
	REQUIRED
)

find_library(LEXILLA_STATIC_LIBRARY
	NAMES liblexilla
	HINTS "../../lexilla/bin"
	REQUIRED
)

ADD_EXECUTABLE(notepad++ ${option} ${src_files} ${include_files} ${rcFiles})


TARGET_LINK_LIBRARIES (notepad++ ${win32_LIBRARIES} ${SCINTILLA_STATIC_LIBRARY} ${LEXILLA_STATIC_LIBRARY})

