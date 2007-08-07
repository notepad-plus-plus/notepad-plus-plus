# this file is part of notepad++
# Copyright (C)2003 Don HO ( donho@altern.org )
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

.SUFFIXES: .cpp
CPP = g++
CC=gcc
RC = windres

#UserDefineDialog_res.o
MAINOBJS = winmain.o Parameters.o Notepad_plus.o Notepad_plus_res.o Utf8_16.o PluginsManager.o

SCINTILLAOBJS = ScintillaEditView.o DocTabView.o FindReplaceDlg.o UserDefineDialog.o Printer.o UserDefineDialog_res.o GoToLineDlg.o

WINCTRLOBJS = TabBar.o ControlsTab.o ToolBar.o StatusBar.o ImageListSet.o FileDialog.o Splitter.o SplitterContainer.o StaticDialog.o \
URLCtrl.o AboutDlg.o RunDlg.o trayIconControler.o shortcut.o ShortcutMapper.o BabyGridWrapper.o BabyGrid.o ShortcutMapper_rc.o \
regExtDlg_res.o regExtDlg.o preference_rc.o preferenceDlg.o WindowsDlg_rc.o SizeableDlg.o WinMgr.o WinRect.o WindowsDlg.o

WORDSTYLESDLG = WordStyleDlg_rc.o WordStyleDlg.o ColourPicker.o ColourPopup_rc.o ColourPopup.o
MISCOBJS = SysMsg.o Process.o
TINYXMLOBJS = tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o

OBJS = $(MAINOBJS) $(SCINTILLAOBJS) $(WINCTRLOBJS) $(MISCOBJS) $(TINYXMLOBJS) $(WORDSTYLESDLG)
#
# the paths
INCLUDEDIR = ../../scintilla/include
SRCDIR = ../src
TARGETDIR=../bin

WINCTRLDIR = $(SRCDIR)/WinControls

SCINTILLADIR = $(SRCDIR)/ScitillaComponent

TINYXMLDIR = $(SRCDIR)/TinyXml

TABBARDIR =   $(WINCTRLDIR)/TabBar
TOOLBARDIR = $(WINCTRLDIR)/ToolBar
STATUSBARDIR = $(WINCTRLDIR)/StatusBar
IMGLSTSETDIR = $(WINCTRLDIR)/ImageListSet
FILEDIALOGDIR = $(WINCTRLDIR)/OpenSaveFileDialog
SPLITTERDIR = $(WINCTRLDIR)/SplitterContainer
STATICDLGDIR = $(WINCTRLDIR)/StaticDialog
ABOUTDLGDIR = $(WINCTRLDIR)/AboutDlg
RUNDLGDIR = $(STATICDLGDIR)/RunDlg
SHORTCUTDIR = $(WINCTRLDIR)/shortcut
MISCDIR = $(SRCDIR)/MISC
SYSMSGDIR = $(MISCDIR)/SysMsg
PROCESSDIR = $(MISCDIR)/Process
REGEXTDIR = $(MISCDIR)/RegExt
PLUGINSMANAGERDIR = $(MISCDIR)/PluginsManager
WORDSTYLESDLGDIR = $(WINCTRLDIR)/ColourPicker
SYSTRAYDIR =   $(WINCTRLDIR)/TrayIcon
CONTEXTMENUDIR =   $(WINCTRLDIR)/ContextMenu
SHORTCUTMAPPERDIR = $(WINCTRLDIR)/Grid
PREFERENCEDLGDIR = $(WINCTRLDIR)/Preference
WINDOWSDLGDIR = $(WINCTRLDIR)/WindowsDlg

PROG = $(TARGETDIR)/npp.exe
FONT_SRC=$(SRCDIR)/font/LINEDRAW.TTF
FONT_TARGET=$(TARGETDIR)/LINEDRAW.TTF

XMLUSERFILE_SRC=$(SRCDIR)/config.xml
XMLUSERFILE_TARGET=$(TARGETDIR)/config.xml
XMLLANGFILE_SRC=$(SRCDIR)/langs.xml
XMLLANGFILE_TARGET=$(TARGETDIR)/langs.xml

INCLUDEFLAGS = -I$(INCLUDEDIR) -I$(SCINTILLADIR) -I$(WINCTRLDIR) \
-I$(TABBARDIR) -I$(TOOLBARDIR) -I$(STATUSBARDIR) -I$(IMGLSTSETDIR) \
-I$(FILEDIALOGDIR) -I$(SPLITTERDIR) -I$(STATICDLGDIR) -I$(ABOUTDLGDIR) \
-I$(SYSMSGDIR) -I$(PROCESSDIR) -I$(MISCDIR) -I$(RUNDLGDIR) -I$(SRCDIR) \
-I$(TINYXMLDIR) -I$(WORDSTYLESDLGDIR) -I$(REGEXTDIR) -I$(PLUGINSMANAGERDIR) \
-I$(SYSTRAYDIR) -I$(SHORTCUTDIR) -I$(CONTEXTMENUDIR) -I$(SHORTCUTMAPPERDIR) \
-I$(PREFERENCEDLGDIR) -I$(WINDOWSDLGDIR)

DEFINEFLAGS = -D_WIN32_IE=0x0600
CFLAGS = -Wall -Os -DNDEBUG
#CFLAGS = -Wall -Os -DDEBUG
LDFLAGS = -mwindows -Wl,-s -Wl,--gc-sections -lcomctl32 -lshlwapi -lodbc32 -lodbccp32 -Os -s
#LDFLAGS = -mwindows -lcomctl32 -lshlwapi -llibshell32

ALL: $(PROG) $(FONT_TARGET) $(XMLLANGFILE_TARGET)

$(PROG):	$(OBJS)
	$(CPP) -Xlinker --subsystem -Xlinker windows -o $@ $(OBJS) $(LDFLAGS)


winmain.o : $(SRCDIR)/winmain.cpp $(SRCDIR)/Notepad_plus.h $(SYSMSGDIR)/SysMsg.h $(CONTEXTMENUDIR)/ContextMenu.h
	$(CPP) $(CFLAGS) -c $(SRCDIR)/winmain.cpp -o $@ $(INCLUDEFLAGS)

Notepad_plus.o : $(SRCDIR)/Notepad_plus.h $(SRCDIR)/Notepad_plus.cpp $(SRCDIR)/Parameters.h \
$(SCINTILLADIR)/ScintillaEditView.h $(SCINTILLADIR)/DocTabView.h $(SCINTILLADIR)/FindReplaceDlg.h \
$(SCINTILLADIR)/UserDefineDialog.h $(SCINTILLADIR)/printer.h $(WINCTRLDIR)/Window.h \
$(TOOLBARDIR)/ToolBar.h $(FILEDIALOGDIR)/FileDialog.h $(SRCDIR)/resource.h \
$(STATUSBARDIR)/StatusBar.h $(ABOUTDLGDIR)/AboutDlg.h $(IMGLSTSETDIR)/ImageListSet.h \
$(STATICDLGDIR)/StaticDialog.h $(SPLITTERDIR)/SplitterContainer.h $(SYSMSGDIR)/SysMsg.h \
$(MISCDIR)/FileNameStringSplitter.h $(SRCDIR)/lastRecentFileList.h $(SRCDIR)/lesDlgs.h $(SHORTCUTDIR)/shortcut.h $(CONTEXTMENUDIR)/ContextMenu.h
	$(CPP) $(CFLAGS) $(DEFINEFLAGS) -c $(SRCDIR)/Notepad_plus.cpp -o $@ $(INCLUDEFLAGS)
#$(PROCESSDIR)/Process.h
Parameters.o : $(SRCDIR)/Parameters.h $(SRCDIR)/Parameters.cpp $(CONTEXTMENUDIR)/ContextMenu.h
	$(CPP) $(CFLAGS) $(DEFINEFLAGS) -c $(SRCDIR)/Parameters.cpp -o $@ $(INCLUDEFLAGS)

Notepad_plus_res.o : $(SRCDIR)/resource.h $(SRCDIR)/Notepad_plus.rc $(SCINTILLADIR)/UserDefineDialog.rc \
$(RUNDLGDIR)/RunDlg.rc $(SHORTCUTDIR)/shortcut.rc
	$(RC)  --include-dir=$(SRCDIR) -o $@ $(SRCDIR)/Notepad_plus.rc

Utf8_16.o : $(SRCDIR)/Utf8_16.h $(SRCDIR)/Utf8_16.cpp
	$(CPP) $(CFLAGS) $(DEFINEFLAGS) -c $(SRCDIR)/Utf8_16.cpp -o $@ $(INCLUDEFLAGS)

ScintillaEditView.o : $(SCINTILLADIR)/ScintillaEditView.h $(SCINTILLADIR)/ScintillaEditView.cpp $(SRCDIR)/Parameters.h \
$(WINCTRLDIR)/Window.h $(INCLUDEDIR)/Scintilla.h $(INCLUDEDIR)/SciLexer.h $(SCINTILLADIR)/Buffer.h  \
$(SCINTILLADIR)/colors.h $(SYSMSGDIR)/SysMsg.h $(STATICDLGDIR)/StaticDialog.h
	$(CPP) $(CFLAGS) -c $(SCINTILLADIR)/ScintillaEditView.cpp -o $@ $(INCLUDEFLAGS)

DocTabView.o : $(SCINTILLADIR)/DocTabView.h $(SCINTILLADIR)/DocTabView.cpp \
$(TABBARDIR)/TabBar.h $(IMGLSTSETDIR)/ImageListSet.h $(SCINTILLADIR)/ScintillaEditView.cpp
	$(CPP) $(CFLAGS) -c $(SCINTILLADIR)/DocTabView.cpp -o $@ $(INCLUDEFLAGS)

FindReplaceDlg.o : $(STATICDLGDIR)/StaticDialog.h $(SRCDIR)/resource.h \
$(SCINTILLADIR)/FindReplaceDlg.h $(SCINTILLADIR)/FindReplaceDlg.cpp
	$(CPP) $(CFLAGS) -c $(SCINTILLADIR)/FindReplaceDlg.cpp -o $@ $(INCLUDEFLAGS)

UserDefineDialog.o : $(STATICDLGDIR)/StaticDialog.h $(SCINTILLADIR)/ScintillaEditView.h \
$(SCINTILLADIR)/UserDefineResource.h  $(SCINTILLADIR)/UserDefineDialog.h \
$(SCINTILLADIR)/UserDefineDialog.cpp
	$(CPP) $(CFLAGS) -c $(SCINTILLADIR)/UserDefineDialog.cpp -o $@ $(INCLUDEFLAGS)

Printer.o : $(SCINTILLADIR)/Printer.h $(SCINTILLADIR)/Printer.cpp $(SCINTILLADIR)/ScintillaEditView.h
	$(CPP) $(CFLAGS) -c $(SCINTILLADIR)/Printer.cpp -o $@ $(INCLUDEFLAGS)

UserDefineDialog_res.o : $(SCINTILLADIR)/UserDefineDialog.rc $(SCINTILLADIR)/UserDefineResource.h
	$(RC) -o $@ $(SCINTILLADIR)/UserDefineDialog.rc

GoToLineDlg.o : $(SCINTILLADIR)/GoToLineDlg.h
	$(CPP) $(CFLAGS) -c $(SCINTILLADIR)/GoToLineDlg.cpp -o $@ $(INCLUDEFLAGS)

TabBar.o : $(WINCTRLDIR)/Window.h $(TABBARDIR)/TabBar.h $(TABBARDIR)/TabBar.cpp $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(TABBARDIR)/TabBar.cpp -o $@ $(INCLUDEFLAGS)

ControlsTab.o : $(TABBARDIR)/ControlsTab.h $(TABBARDIR)/ControlsTab.cpp
	$(CPP) $(CFLAGS) $(DEFINEFLAGS) -c $(TABBARDIR)/ControlsTab.cpp -o $@ $(INCLUDEFLAGS)

ToolBar.o: $(WINCTRLDIR)/Window.h $(TOOLBARDIR)/ToolBar.h $(TOOLBARDIR)/ToolBar.cpp \
$(IMGLSTSETDIR)/ImageListSet.h $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(TOOLBARDIR)/ToolBar.cpp -o $@ $(INCLUDEFLAGS)

StatusBar.o: $(WINCTRLDIR)/Window.h $(STATUSBARDIR)/StatusBar.h $(STATUSBARDIR)/StatusBar.cpp  $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(STATUSBARDIR)/StatusBar.cpp -o $@ $(INCLUDEFLAGS)

ImageListSet.o: $(IMGLSTSETDIR)/ImageListSet.h  $(IMGLSTSETDIR)/ImageListSet.cpp
	$(CPP) $(CFLAGS) -c $(IMGLSTSETDIR)/ImageListSet.cpp -o $@ $(INCLUDEFLAGS)

FileDialog.o: $(FILEDIALOGDIR)/FileDialog.h  $(FILEDIALOGDIR)/FileDialog.cpp
	$(CPP) $(CFLAGS) -c $(FILEDIALOGDIR)/FileDialog.cpp -o $@ $(INCLUDEFLAGS)

Splitter.o: $(WINCTRLDIR)/Window.h $(SPLITTERDIR)/Splitter.h $(SPLITTERDIR)/Splitter.cpp $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(SPLITTERDIR)/Splitter.cpp -o $@ $(INCLUDEFLAGS)

SplitterContainer.o: $(WINCTRLDIR)/Window.h $(SPLITTERDIR)/SplitterContainer.h $(SPLITTERDIR)/SplitterContainer.cpp $(SPLITTERDIR)/Splitter.h $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(SPLITTERDIR)/SplitterContainer.cpp -o $@ $(INCLUDEFLAGS)

StaticDialog.o: $(WINCTRLDIR)/Window.h $(STATICDLGDIR)/StaticDialog.h $(STATICDLGDIR)/StaticDialog.cpp
	$(CPP) $(CFLAGS) -c $(STATICDLGDIR)/StaticDialog.cpp -o $@ $(INCLUDEFLAGS)

URLCtrl.o: $(WINCTRLDIR)/Window.h $(ABOUTDLGDIR)/URLCtrl.h $(ABOUTDLGDIR)/URLCtrl.cpp
	$(CPP) $(CFLAGS) -c $(ABOUTDLGDIR)/URLCtrl.cpp -o $@ $(INCLUDEFLAGS)

AboutDlg.o: $(WINCTRLDIR)/Window.h $(ABOUTDLGDIR)/AboutDlg.h $(ABOUTDLGDIR)/AboutDlg.cpp URLCtrl.o
	$(CPP) $(CFLAGS) -c $(ABOUTDLGDIR)/AboutDlg.cpp -o $@ $(INCLUDEFLAGS)

RunDlg.o: $(WINCTRLDIR)/Window.h $(RUNDLGDIR)/RunDlg.h $(RUNDLGDIR)/RunDlg.cpp $(RUNDLGDIR)/RunDlg.rc
	$(CPP) $(CFLAGS) -c $(RUNDLGDIR)/RunDlg.cpp -o $@ $(INCLUDEFLAGS)

shortcut.o: $(WINCTRLDIR)/Window.h $(SHORTCUTDIR)/shortcut.h $(SHORTCUTDIR)/shortcut.rc
	$(CPP) $(CFLAGS) -c $(SHORTCUTDIR)/shortcut.cpp -o $@ $(INCLUDEFLAGS)

trayIconControler.o: $(SYSTRAYDIR)/trayIconControler.h
	$(CPP) $(CFLAGS) -c $(SYSTRAYDIR)/trayIconControler.cpp -o $@ $(INCLUDEFLAGS)

RunDlg_res.o : $(RUNDLGDIR)/RunDlg_rc.h $(RUNDLGDIR)/RunDlg.rc
	$(RC)  --include-dir=$(SRCDIR) -o $@ $(RunDlg)/RunDlg.rc

SysMsg.o: $(SYSMSGDIR)/SysMsg.h $(SYSMSGDIR)/SysMsg.cpp
	$(CPP) $(CFLAGS) -c $(SYSMSGDIR)/SysMsg.cpp -o $@ $(INCLUDEFLAGS)

Process.o: $(PROCESSDIR)/Process.h $(PROCESSDIR)/Process.cpp
	$(CPP) $(CFLAGS) -c $(PROCESSDIR)/Process.cpp -o $@ $(INCLUDEFLAGS)

tinystr.o: $(TINYXMLDIR)/tinystr.h $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinystr.cpp -o $@ $(INCLUDEFLAGS)

tinyxml.o: $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinyxml.cpp -o $@ $(INCLUDEFLAGS)

tinyxmlerror.o: $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinyxmlerror.cpp -o $@ $(INCLUDEFLAGS)

tinyxmlparser.o: $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinyxmlparser.cpp -o $@ $(INCLUDEFLAGS)
#WebCtrl.o:
#WebControl.o:
WordStyleDlg_rc.o : $(WORDSTYLESDLGDIR)/wordstyledlgres.h
	$(RC)  --include-dir=$(SRCDIR) -o $@ $(WORDSTYLESDLGDIR)/WordStyleDlg.rc

WordStyleDlg.o : $(WORDSTYLESDLGDIR)/WordStyleDlg.h $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(WORDSTYLESDLGDIR)/WordStyleDlg.cpp -o $@ $(INCLUDEFLAGS)

ColourPicker.o : $(WORDSTYLESDLGDIR)/ColourPicker.h $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(WORDSTYLESDLGDIR)/ColourPicker.cpp -o $@ $(INCLUDEFLAGS)

ColourPopup_rc.o : $(WORDSTYLESDLGDIR)/ColourPopupResource.h
	$(RC)  --include-dir=$(SRCDIR) -o $@ $(WORDSTYLESDLGDIR)/ColourPopup.rc

ColourPopup.o : $(WORDSTYLESDLGDIR)/ColourPopup.h $(SYSMSGDIR)/SysMsg.h
	$(CPP) $(CFLAGS) -c $(WORDSTYLESDLGDIR)/ColourPopup.cpp -o $@ $(INCLUDEFLAGS)

regExtDlg_res.o : $(REGEXTDIR)/regExtDlgRc.h $(REGEXTDIR)/regExtDlg.rc
	$(RC)  --include-dir=$(SRCDIR) -o $@ $(SRCDIR)/Notepad_plus.rc

regExtDlg.o : $(REGEXTDIR)/regExtDlg.h $(REGEXTDIR)/regExtDlg.cpp
	$(CPP) $(CFLAGS) -c $(REGEXTDIR)/regExtDlg.cpp -o $@ $(INCLUDEFLAGS)

PluginsManager.o : $(PLUGINSMANAGERDIR)/PluginsManager.h $(PLUGINSMANAGERDIR)/PluginInterface.h
	$(CPP) $(CFLAGS) -c $(PLUGINSMANAGERDIR)/PluginsManager.cpp -o $@ $(INCLUDEFLAGS)

BabyGrid.o : $(SHORTCUTMAPPERDIR)/BabyGrid.h
	$(CPP) $(CFLAGS) -c $(SHORTCUTMAPPERDIR)/BabyGrid.cpp -o $@ $(INCLUDEFLAGS)

BabyGridWrapper.o : $(SHORTCUTMAPPERDIR)/BabyGridWrapper.h $(SHORTCUTMAPPERDIR)/BabyGrid.h
	$(CPP) $(CFLAGS) -c $(SHORTCUTMAPPERDIR)/BabyGridWrapper.cpp -o $@ $(INCLUDEFLAGS)

ShortcutMapper_rc.o : $(SHORTCUTMAPPERDIR)/ShortcutMapper.rc $(SHORTCUTMAPPERDIR)/ShortcutMapper_rc.h
	$(RC) -o $@ $(SHORTCUTMAPPERDIR)/ShortcutMapper.rc

ShortcutMapper.o : $(SHORTCUTMAPPERDIR)/ShortcutMapper.h
	$(CPP) $(CFLAGS) -c $(SHORTCUTMAPPERDIR)/ShortcutMapper.cpp -o $@ $(INCLUDEFLAGS)

preference_rc.o : $(PREFERENCEDLGDIR)/preference_rc.h $(PREFERENCEDLGDIR)/preference.rc
	$(RC) -o $@ $(PREFERENCEDLGDIR)/preference.rc

preferenceDlg.o : $(PREFERENCEDLGDIR)/preferenceDlg.h
	$(CPP) $(CFLAGS) -c $(PREFERENCEDLGDIR)/preferenceDlg.cpp -o $@ $(INCLUDEFLAGS)

WindowsDlg_rc.o : $(WINDOWSDLGDIR)/WindowsDlgRc.h $(WINDOWSDLGDIR)/WindowsDlg.rc
	$(RC) -o $@ $(WINDOWSDLGDIR)/WindowsDlg.rc

SizeableDlg.o : $(WINDOWSDLGDIR)/SizeableDlg.h
	$(CPP) $(CFLAGS) -c $(WINDOWSDLGDIR)/SizeableDlg.cpp -o $@ $(INCLUDEFLAGS)
	
WinMgr.o : $(WINDOWSDLGDIR)/WinMgr.h
	$(CPP) $(CFLAGS) -c $(WINDOWSDLGDIR)/WinMgr.cpp -o $@ $(INCLUDEFLAGS)

WinRect.o : $(WINDOWSDLGDIR)/WinMgr.h
	$(CPP) $(CFLAGS) -c $(WINDOWSDLGDIR)/WinRect.cpp -o $@ $(INCLUDEFLAGS)

WindowsDlg.o : $(WINDOWSDLGDIR)/WindowsDlg.h
	$(CPP) $(CFLAGS) -c $(WINDOWSDLGDIR)/WindowsDlg.cpp -o $@ $(INCLUDEFLAGS)

$(FONT_TARGET): $(FONT_SRC)
	cp $^ $@

$(XMLUSERFILE_TARGET) : $(XMLUSERFILE_SRC)
	cp $^ $@

$(XMLLANGFILE_TARGET) : $(XMLLANGFILE_SRC)
	cp $^ $@


clean:
	rm -f $(OBJS) $(PROG) $(XMLLANGFILE_TARGET) $(FONT_TARGET)

