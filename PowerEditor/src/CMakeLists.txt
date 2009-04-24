# @file:<dictaolib-root>/CMakeLists.txt

# TODO
# - Move defs XX_INCLUDE_DIR, XX_LIBRARIES, XX_LIBRARY_DIRS to the subdirector?es'
#   scripts. I could not find a way to export a variables defined in a subscript
#   so as a workaround, I define the variables myself in the top script. This is
#   not optimal.

# 

PROJECT(Notepad++)

SET(projIncludDir	../../scintilla/include/
					./
					./ScitillaComponent/
					./WinControls/
					./WinControls/AboutDlg/
					./WinControls/ColourPicker/
					./WinControls/ContextMenu/
					./WinControls/DockingWnd/
					./WinControls/Grid/
					./WinControls/ImageListSet/
					./WinControls/OpenSaveFileDialog/
					./WinControls/Preference/
					./WinControls/shortcut/
					./WinControls/SplitterContainer/
					./WinControls/StaticDialog/
					./WinControls/StaticDialog/RunDlg
					./WinControls/StatusBar/
					./WinControls/TabBar/
					./WinControls/TaskList/
					./WinControls/ToolBar/
					./WinControls/TrayIcon/
					./WinControls/TreeView/
					./WinControls/WindowsDlg/
					./MISC/
					./MISC/PluginsManager/
					./MISC/RegExt/
					./MISC/SysMsg/
					./TinyXml/)

set(parameters ./Parameters.h ./Parameters.cpp)

set(scintilla ./ScitillaComponent/ScintillaEditView.h ./ScitillaComponent/ScintillaEditView.cpp)
set(buffer	 ./ScitillaComponent/ScintillaEditView.h ./ScitillaComponent/ScintillaEditView.cpp)

set(tinystr ./TinyXml/tinystr.h ./TinyXml/tinystr.cpp)
set(tinyxml ./TinyXml/tinyxml.h ./TinyXml/tinyxml.cpp)
set(tinyxmlerror ./TinyXml/tinyxmlerror.cpp)
set(tinyxmlparser ./TinyXml/tinyxmlparser.cpp)
SET (tinyxmlObjs ${tinystr} ${tinyxml} ${tinyxmlerror} ${tinyxmlparser})

SET(objs	./winmain.cpp
			./Notepad_plus.cpp
			./Notepad_plus.h

			./UniConversion.cpp
			./UniConversion.h
			./Utf8_16.cpp
			./Utf8_16.h

			
			
			./ScitillaComponent/DocTabView.cpp
			./ScitillaComponent/DocTabView.h
			./ScitillaComponent/FindReplaceDlg.cpp
			./ScitillaComponent/FindReplaceDlg.h
			./ScitillaComponent/GoToLineDlg.cpp
			./ScitillaComponent/GoToLineDlg.h
			./ScitillaComponent/Printer.cpp
			./ScitillaComponent/Printer.h
			./ScitillaComponent/UserDefineDialog.cpp
			./ScitillaComponent/UserDefineDialog.h
			./WinControls/AboutDlg/AboutDlg.cpp
			./WinControls/AboutDlg/AboutDlg.h
			./WinControls/AboutDlg/URLCtrl.cpp
			./WinControls/AboutDlg/URLCtrl.h
			./WinControls/ColourPicker/ColourPicker.cpp
			./WinControls/ColourPicker/ColourPicker.h
			./WinControls/ColourPicker/ColourPopup.cpp
			./WinControls/ColourPicker/ColourPopup.h
			#./WinControls/ColourPicker/FontPreviewCombo.cpp
			./WinControls/ColourPicker/WordStyleDlg.cpp
			./WinControls/ColourPicker/WordStyleDlg.h
			./WinControls/DockingWnd/DockingCont.cpp
			./WinControls/DockingWnd/DockingCont.h
			./WinControls/DockingWnd/DockingManager.cpp
			./WinControls/DockingWnd/DockingManager.h
			./WinControls/DockingWnd/DockingSplitter.cpp
			./WinControls/DockingWnd/DockingSplitter.h
			#./WinControls/DockingWnd/DropData.cpp
			./WinControls/DockingWnd/Gripper.cpp
			./WinControls/DockingWnd/Gripper.h
			./WinControls/DockingWnd/common_func.cpp
			./WinControls/DockingWnd/common_func.h
			./WinControls/Grid/BabyGrid.cpp
			./WinControls/Grid/BabyGrid.h
			./WinControls/Grid/BabyGridWrapper.cpp
			./WinControls/Grid/BabyGridWrapper.h
			./WinControls/Grid/ShortcutMapper.cpp
			./WinControls/Grid/ShortcutMapper.h
			./WinControls/ImageListSet/ImageListSet.cpp
			./WinControls/ImageListSet/ImageListSet.h
			./WinControls/OpenSaveFileDialog/FileDialog.cpp
			./WinControls/OpenSaveFileDialog/FileDialog.h
			./WinControls/Preference/preferenceDlg.cpp
			./WinControls/Preference/preferenceDlg.h
			./WinControls/shortcut/shortcut.cpp
			./WinControls/shortcut/shortcut.h
			./WinControls/shortcut/RunMacroDlg.cpp
			./WinControls/shortcut/RunMacroDlg.h
			./WinControls/SplitterContainer/Splitter.cpp
			./WinControls/SplitterContainer/Splitter.h
			./WinControls/SplitterContainer/SplitterContainer.cpp
			./WinControls/SplitterContainer/SplitterContainer.h
			./WinControls/StaticDialog/StaticDialog.cpp
			./WinControls/StaticDialog/StaticDialog.h
			./WinControls/StaticDialog/RunDlg/RunDlg.cpp
			./WinControls/StaticDialog/RunDlg/RunDlg.h
			./WinControls/StatusBar/StatusBar.cpp
			./WinControls/StatusBar/StatusBar.h
			./WinControls/TabBar/TabBar.cpp
			./WinControls/TabBar/TabBar.h
			./WinControls/TabBar/ControlsTab.cpp
			./WinControls/TabBar/ControlsTab.h
			./WinControls/TaskList/TaskList.cpp
			./WinControls/TaskList/TaskList.h
			./WinControls/ToolBar/ToolBar.cpp
			./WinControls/ToolBar/ToolBar.h
			./WinControls/TrayIcon/trayIconControler.cpp
			./WinControls/TrayIcon/trayIconControler.h
			./WinControls/TreeView/TreeView.cpp
			./WinControls/TreeView/TreeView.h
			./WinControls/WindowsDlg/SizeableDlg.cpp
			./WinControls/WindowsDlg/SizeableDlg.h
			./WinControls/WindowsDlg/WindowsDlg.cpp
			./WinControls/WindowsDlg/WindowsDlg.h
			./WinControls/WindowsDlg/WinMgr.cpp
			./WinControls/WindowsDlg/WinMgr.h
			./WinControls/WindowsDlg/WinRect.cpp
			./MISC/PluginsManager/PluginsManager.cpp
			./MISC/PluginsManager/PluginsManager.h
			./MISC/RegExt/regExtDlg.cpp
			./MISC/RegExt/regExtDlg.h
			./MISC/SysMsg/SysMsg.cpp
			./MISC/SysMsg/SysMsg.h
			${tinyxmlObjs}

)


SET(rcFiles	./Notepad_plus.rc
			./ScitillaComponent/FindReplaceDlg.rc
			./ScitillaComponent/UserDefineDialog.rc
			./WinControls/ColourPicker/ColourPopup.rc
			./WinControls/ColourPicker/WordStyleDlg.rc
			./WinControls/DockingWnd/DockingGUIWidget.rc
			./WinControls/Grid/ShortcutMapper.rc
			./WinControls/Preference/preference.rc
			./WinControls/shortcut/RunMacroDlg.rc
			./WinControls/shortcut/shortcut.rc
			./WinControls/StaticDialog/RunDlg/RunDlg.rc
			./WinControls/TaskList/TaskListDlg.rc
			./WinControls/WindowsDlg/WindowsDlg.rc

)
SET(vendorIncludDir ../../../vendor/Gemalto/Current/SDK_IASAPI/Output/include)

IF (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	SET(option WIN32)
	SET(win32_LIBRARIES comctl32.lib shlwapi.lib shell32.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib)
	#SET(defs -DUNICODE -D_UNICODE)
ENDIF (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

#ADD_DEFINITIONS(${defs})

INCLUDE_DIRECTORIES(${projIncludDir})

ADD_EXECUTABLE(notepad++ ${option} ${objs} ${rcFiles})

TARGET_LINK_LIBRARIES (notepad++ ${win32_LIBRARIES})

