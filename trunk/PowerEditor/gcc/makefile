# this file is part of notepad++
# Copyright (C)2008 Harry Bruin <harrybharry@users.sourceforge.net>
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

# Directories:
NPP_DIR			= ../src
SCI_DIR			= ../../scintilla/include
GCC_DIR			= .
GCCINCLUDE_DIR  = $(GCC_DIR)/include

#MISC:
MISC_DIR		= $(NPP_DIR)/MISC
 COMMON_DIR		= $(MISC_DIR)/Common
 EXCEPTION_DIR	= $(MISC_DIR)/Exception
 PLUGINS_DIR	= $(MISC_DIR)/PluginsManager
 PROCESS_DIR	= $(MISC_DIR)/Process
 REGEXT_DIR		= $(MISC_DIR)/RegExt

#ScintillaComponent:
SCINT_DIR		= $(NPP_DIR)/ScitillaComponent

#TinyXml
TIXML_DIR		= $(NPP_DIR)/TinyXml

#WinControls
WIN_DIR			= $(NPP_DIR)/WinControls
 ABOUT_DIR		= $(WIN_DIR)/AboutDlg
 CONTEXT_DIR	= $(WIN_DIR)/ContextMenu
 COLOUR_DIR		= $(WIN_DIR)/ColourPicker
 DOCKING_DIR	= $(WIN_DIR)/DockingWnd
 GRID_DIR		= $(WIN_DIR)/Grid
 IMLIST_DIR		= $(WIN_DIR)/ImageListSet
 OPENSAVE_DIR	= $(WIN_DIR)/OpenSaveFileDialog
 # PANNER_DIR		= $(WIN_DIR)/Panner
 PREFERENCE_DIR	= $(WIN_DIR)/Preference
 SHORTCUT_DIR	= $(WIN_DIR)/shortcut
 SPLITTER_DIR	= $(WIN_DIR)/SplitterContainer
 STATICDLG_DIR	= $(WIN_DIR)/StaticDialog
  RUNDLG_DIR	= $(STATICDLG_DIR)/RunDlg
 STATUSBAR_DIR	= $(WIN_DIR)/StatusBar
 TABBAR_DIR		= $(WIN_DIR)/TabBar
 TASKLIST_DIR	= $(WIN_DIR)/TaskList
 TOOLBAR_DIR	= $(WIN_DIR)/ToolBar
 TOOLTIP_DIR	= $(WIN_DIR)/ToolTip
 TRAYICON_DIR	= $(WIN_DIR)/TrayIcon
 TREEVIEW_DIR	= $(WIN_DIR)/TreeView
 WINDOWSDLG_DIR	= $(WIN_DIR)/WindowsDlg


# Sources:
SRC_NPP			= $(wildcard $(NPP_DIR)/*.cpp)
SRC_GCCINCLUDE  = $(wildcard $(GCCINCLUDE_DIR)/*.cpp)

#MISC
SRC_MISC		= $(wildcard $(MISC_DIR)/*.cpp)
 SRC_COMMON		= $(wildcard $(COMMON_DIR)/*.cpp)
 SRC_EXCEPTION	= $(wildcard $(EXCEPTION_DIR)/*.cpp)
 SRC_PLUGINS	= $(wildcard $(PLUGINS_DIR)/*.cpp)
 SRC_PROCESS	= $(wildcard $(PROCESS_DIR)/*.cpp)
 SRC_REGEXT		= $(wildcard $(REGEXT_DIR)/*.cpp)

#ScintillaComponent
SRC_SCINT		= $(wildcard $(SCINT_DIR)/*.cpp)

#TinyXml
SRC_TIXML		= $(wildcard $(TIXML_DIR)/*.cpp)

#WinControls
SRC_WIN			= $(wildcard $(WIN_DIR)/*.cpp)
 SRC_ABOUT		= $(wildcard $(ABOUT_DIR)/*.cpp)
 SRC_CONTEXT	= $(wildcard $(CONTEXT_DIR)/*.cpp)
 SRC_COLOUR		= $(wildcard $(COLOUR_DIR)/*.cpp)
 SRC_DOCKING	= $(wildcard $(DOCKING_DIR)/*.cpp)
 SRC_GRID		= $(wildcard $(GRID_DIR)/*.cpp)
 SRC_IMLIST		= $(wildcard $(IMLIST_DIR)/*.cpp)
 SRC_OPENSAVE	= $(wildcard $(OPENSAVE_DIR)/*.cpp)
 SRC_PREFERENCE	= $(wildcard $(PREFERENCE_DIR)/*.cpp)
 SRC_SHORTCUT	= $(wildcard $(SHORTCUT_DIR)/*.cpp)
 SRC_SPLITTER	= $(wildcard $(SPLITTER_DIR)/*.cpp)
 SRC_STATICDLG	= $(wildcard $(STATICDLG_DIR)/*.cpp)
  SRC_RUNDLG	= $(wildcard $(RUNDLG_DIR)/*.cpp)
 SRC_STATUSBAR	= $(wildcard $(STATUSBAR_DIR)/*.cpp)
 SRC_TABBAR		= $(wildcard $(TABBAR_DIR)/*.cpp)
 SRC_TASKLIST	= $(wildcard $(TASKLIST_DIR)/*.cpp)
 SRC_TOOLBAR	= $(wildcard $(TOOLBAR_DIR)/*.cpp)
 SRC_TOOLTIP	= $(wildcard $(TOOLTIP_DIR)/*.cpp)
 SRC_TRAYICON	= $(wildcard $(TRAYICON_DIR)/*.cpp)
 SRC_TREEVIEW	= $(wildcard $(TREEVIEW_DIR)/*.cpp)
 SRC_WINDOWSDLG	= $(wildcard $(WINDOWSDLG_DIR)/*.cpp)

# Objects:
OBJ_NPP			= $(patsubst %.cpp,%.o,$(SRC_NPP))
OBJ_GCCINCLUDE  = $(patsubst %.cpp,%.o,$(SRC_GCCINCLUDE))

#MISC
OBJ_MISC		= $(patsubst %.cpp,%.o,$(SRC_MISC))
 OBJ_COMMON		= $(patsubst %.cpp,%.o,$(SRC_COMMON))
 OBJ_EXCEPTION	= $(patsubst %.cpp,%.o,$(SRC_EXCEPTION))
 OBJ_PLUGINS	= $(patsubst %.cpp,%.o,$(SRC_PLUGINS))
 OBJ_PROCESS	= $(patsubst %.cpp,%.o,$(SRC_PROCESS))
 OBJ_REGEXT		= $(patsubst %.cpp,%.o,$(SRC_REGEXT))

#ScintillaComponent
OBJ_SCINT		= $(patsubst %.cpp,%.o,$(SRC_SCINT))

#TinyXml
OBJ_TIXML		= $(patsubst %.cpp,%.o,$(SRC_TIXML))

#WinControls
OBJ_WIN			= $(patsubst %.cpp,%.o,$(SRC_WIN))
 OBJ_ABOUT		= $(patsubst %.cpp,%.o,$(SRC_ABOUT))
 OBJ_CONTEXT	= $(patsubst %.cpp,%.o,$(SRC_CONTEXT))
 OBJ_COLOUR		= $(patsubst %.cpp,%.o,$(SRC_COLOUR))
 OBJ_DOCKING	= $(patsubst %.cpp,%.o,$(SRC_DOCKING))
 OBJ_GRID		= $(patsubst %.cpp,%.o,$(SRC_GRID))
 OBJ_IMLIST		= $(patsubst %.cpp,%.o,$(SRC_IMLIST))
 OBJ_OPENSAVE	= $(patsubst %.cpp,%.o,$(SRC_OPENSAVE))
 OBJ_PREFERENCE	= $(patsubst %.cpp,%.o,$(SRC_PREFERENCE))
 OBJ_SHORTCUT	= $(patsubst %.cpp,%.o,$(SRC_SHORTCUT))
 OBJ_SPLITTER	= $(patsubst %.cpp,%.o,$(SRC_SPLITTER))
 OBJ_STATICDLG	= $(patsubst %.cpp,%.o,$(SRC_STATICDLG))
  OBJ_RUNDLG	= $(patsubst %.cpp,%.o,$(SRC_RUNDLG))
 OBJ_STATUSBAR	= $(patsubst %.cpp,%.o,$(SRC_STATUSBAR))
 OBJ_TABBAR		= $(patsubst %.cpp,%.o,$(SRC_TABBAR))
 OBJ_TASKLIST	= $(patsubst %.cpp,%.o,$(SRC_TASKLIST))
 OBJ_TOOLBAR	= $(patsubst %.cpp,%.o,$(SRC_TOOLBAR))
 OBJ_TOOLTIP	= $(patsubst %.cpp,%.o,$(SRC_TOOLTIP))
 OBJ_TRAYICON	= $(patsubst %.cpp,%.o,$(SRC_TRAYICON))
 OBJ_TREEVIEW	= $(patsubst %.cpp,%.o,$(SRC_TREEVIEW))
 OBJ_WINDOWSDLG	= $(patsubst %.cpp,%.o,$(SRC_WINDOWSDLG))

# Collections
DIRS_WIN		= $(WIN_DIR) $(ABOUT_DIR) $(CONTEXT_DIR) $(COLOUR_DIR) $(DOCKING_DIR) $(GRID_DIR) $(IMLIST_DIR) $(OPENSAVE_DIR) $(PREFERENCE_DIR) $(SHORTCUT_DIR) $(SPLITTER_DIR) $(STATICDLG_DIR) $(RUNDLG_DIR) $(STATUSBAR_DIR) $(TABBAR_DIR) $(TASKLIST_DIR) $(TOOLBAR_DIR) $(TOOLTIP_DIR) $(TRAYICON_DIR) $(TREEVIEW_DIR) $(WINDOWSDLG_DIR)
DIRS_TIXML		= $(TIXML_DIR)
DIRS_SCINT		= $(SCINT_DIR)
DIRS_MISC		= $(MISC_DIR) $(COMMON_DIR) $(EXCEPTION_DIR) $(PLUGINS_DIR) $(PROCESS_DIR) $(REGEXT_DIR)
DIRS			= $(NPP_DIR) $(DIRS_WIN) $(DIRS_TIXML) $(DIRS_SCINT) $(DIRS_MISC) $(SCI_DIR)

SRCS_WIN		= $(SRC_WIN) $(SRC_ABOUT) $(SRC_CONTEXT) $(SRC_COLOUR) $(SRC_DOCKING) $(SRC_GRID) $(SRC_IMLIST) $(SRC_OPENSAVE) $(SRC_PREFERENCE) $(SRC_SHORTCUT) $(SRC_SPLITTER) $(SRC_STATICDLG) $(SRC_RUNDLG) $(SRC_STATUSBAR) $(SRC_TABBAR) $(SRC_TASKLIST) $(SRC_TOOLBAR) $(SRC_TOOLTIP) $(SRC_TRAYICON) $(SRC_TREEVIEW) $(SRC_WINDOWSDLG)
SRCS_TIXML		= $(SRC_TIXML)
SRCS_SCINT		= $(SRC_SCINT)
SRCS_MISC		= $(SRC_MISC) $(SRC_COMMON) $(SRC_EXCEPTION) $(SRC_PLUGINS) $(SRC_PROCESS) $(SRC_REGEXT)
SRCS			= $(SRC_NPP) $(SRCS_WIN) $(SRCS_TIXML) $(SRCS_SCINT) $(SRCS_MISC) $(SRC_GCCINCLUDE)

OBJS_WIN		= $(OBJ_WIN) $(OBJ_ABOUT) $(OBJ_CONTEXT) $(OBJ_COLOUR) $(OBJ_DOCKING) $(OBJ_GRID) $(OBJ_IMLIST) $(OBJ_OPENSAVE) $(OBJ_PREFERENCE) $(OBJ_SHORTCUT) $(OBJ_SPLITTER) $(OBJ_STATICDLG) $(OBJ_RUNDLG) $(OBJ_STATUSBAR) $(OBJ_TABBAR) $(OBJ_TASKLIST) $(OBJ_TOOLBAR) $(OBJ_TOOLTIP) $(OBJ_TRAYICON) $(OBJ_TREEVIEW) $(OBJ_WINDOWSDLG)
OBJS_TIXML		= $(OBJ_TIXML)
OBJS_SCINT		= $(OBJ_SCINT)
OBJS_MISC		= $(OBJ_MISC) $(OBJ_COMMON) $(OBJ_EXCEPTION) $(OBJ_PLUGINS) $(OBJ_PROCESS) $(OBJ_REGEXT)
OBJS			= $(OBJ_NPP) $(OBJS_WIN) $(OBJS_TIXML) $(OBJS_SCINT) $(OBJS_MISC) $(OBJ_GCCINCLUDE)

# Main resource file
SRC_RES			= ./resources.rc
OBJ_RES			= $(patsubst %.rc,%.res,$(SRC_RES))

# Parameters
INCLUDESPECIAL	= -include./include/various.h -includeCommon.h
#    Comment this out for ANSI build
UNICODE         = -DUNICODE -D_UNICODE

CXX				= g++
#CXXFLAGS		= -O2 $(INCLUDESPECIAL)
CXXFLAGS		= $(INCLUDESPECIAL) -DTIXML_USE_STL $(UNICODE)
INCLUDES		= $(patsubst %,-I%,$(DIRS)) -I./include
LDFLAGS			= -Wl,--subsystem,windows
LIBS			= -lcomdlg32 -lcomctl32 -lgdi32 -lole32 -loleacc -lshell32 -lshlwapi

RC				= windres

OUT_NPP			= NotepadPP.exe

EXEC			= ../bin/$(OUT_NPP)

all: NotepadPP

# Main Notepad++ rule
NotepadPP: $(OBJS) $(OBJ_RES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(OBJS) $(OBJ_RES) -o $(EXEC) $(LIBS)

NotepadPPUnicode: $(OBJS) $(OBJ_RES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $(OBJS) $(OBJ_RES) -o $(EXEC) $(LIBS)
	
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.res: %.rc
	$(RC) $(INCLUDES) $(UNICODE) --input=$< --output=$@ --input-format=rc --output-format=coff

# Cleanup
clean:
	@delete.bat
