# This file is part of Notepad++ project
# Copyright (C)2021 Ivan U7n <jprofic@yandex.ru>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# at your option any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Definitions:
SOURCE_DIR = ../src
SOURCE_EXCLUDE = $(SOURCE_DIR)/tools/%
TARGET_DIR = ../bin
TARGET_BASE = NotepadPP
TARGET_SUFFIX = .exe
CONFIG_FILES = langs.model.xml stylers.model.xml shortcuts.xml contextMenu.xml
GCC_DIR = .
SCINTILLA_DIR = ../../scintilla

CXX = $(CROSS_COMPILE)g++
CXXFLAGS += -include$(GCC_DIR)/include/various.h -std=c++17 -fpermissive -Wno-conversion-null
RC = $(CROSS_COMPILE)windres
#RCFLAGS +=
LD = $(CROSS_COMPILE)g++
LDFLAGS += -municode -mwindows

DEFINES = -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600 -DTIXML_USE_STL -DTIXMLA_USE_STL
LIBRARIES = -lcomctl32 -lcrypt32 -ldbghelp -lole32 -lsensapi -lshlwapi -luuid -luxtheme -lversion -lwininet -lwintrust
SCINTILLA_TARGET = $(SCINTILLA_DIR)/bin/libscilexer.a
SCINTILLA_LIBRARIES = -limm32 -lmsimg32 -loleaut32

ifndef DEBUG
BUILD_TYPE = release
CXXFLAGS += -O2 -Os
LDFLAGS += -s
DEFINES += -DNDEBUG
else
BUILD_TYPE = debug
CXXFLAGS += -g -Wall -Wpedantic -Wconversion-null
#DEFINES += -DDEBUG
endif

BUILD_DIR = $(GCC_DIR)/$(CROSS_COMPILE)$(BUILD_TYPE)
TARGET_BINARY = $(TARGET_DIR)/$(TARGET_BASE)-$(CROSS_COMPILE)$(BUILD_TYPE)$(TARGET_SUFFIX)

ifndef VERBOSE
.SILENT:
endif

ifdef windir
MKDIR = $(if $(wildcard $(dir $(SHELL))mkdir.exe),$(dir $(SHELL))mkdir.exe -p,mkdir)
RMDIR = $(if $(wildcard $(dir $(SHELL))rm.exe),$(dir $(SHELL))rm.exe -rf,rmdir /q /s)
CP = $(if $(wildcard $(dir $(SHELL))cp.exe),$(dir $(SHELL))cp.exe,copy /y)
RM = $(if $(wildcard $(dir $(SHELL))rm.exe),$(dir $(SHELL))rm.exe -f,del /q)
else
MKDIR = mkdir -p
RMDIR = rm -rf
CP = cp
RM = rm -f
endif

subtree-list = $(foreach d,$(wildcard $(1:=/*)),$(call subtree-list,$d) $d)
normalize = $(if $(windir),$(subst /,\,$1),$1)

GCC_ENTRIES = $(call subtree-list,$(GCC_DIR))
SOURCE_ENTRIES = $(filter-out $(SOURCE_EXCLUDE),$(call subtree-list,$(SOURCE_DIR)))

INCLUDES += $(addprefix -I,$(sort $(dir $(filter %.h %.hpp,$(SOURCE_ENTRIES)))))
INCLUDES += -I$(SCINTILLA_DIR)/include

vpath %.cpp $(GCC_DIR) $(SOURCE_DIR)
CXXS_IN += $(patsubst $(GCC_DIR)/%,%,$(filter %.cpp,$(GCC_ENTRIES)))
CXXS_IN += $(patsubst $(SOURCE_DIR)/%,%,$(filter %.cpp,$(SOURCE_ENTRIES)))
CXXS_OUT = $(sort $(CXXS_IN:%.cpp=$(BUILD_DIR)/%.o))

vpath %.rc $(GCC_DIR) $(SOURCE_DIR)
RCS_IN += $(patsubst $(GCC_DIR)/%,%,$(filter %.rc,$(GCC_ENTRIES)))
RCS_IN += $(patsubst $(SOURCE_DIR)/%,%,$(filter %.rc,$(SOURCE_ENTRIES)))
RCS_OUT = $(sort $(RCS_IN:%.rc=$(BUILD_DIR)/%.res))

CONFIGS_IN = $(addprefix $(SOURCE_DIR)/,$(CONFIG_FILES))
CONFIGS_OUT = $(addprefix $(TARGET_DIR)/,$(CONFIG_FILES))

# Rules:
all: $(TARGET_BINARY) $(CONFIGS_OUT)

$(TARGET_BINARY): $(CXXS_OUT) $(RCS_OUT) $(SCINTILLA_TARGET)
	@echo linking $@
	$(LD) $(LDFLAGS) $^ $(LIBRARIES) $(SCINTILLA_LIBRARIES) -static -o $@

$(CXXS_OUT): | $(BUILD_DIR)
$(CXXS_OUT): $(BUILD_DIR)/%.o: %.cpp
	@echo compiling $<
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -MMD -c -o $@ $<

$(RCS_OUT): | $(BUILD_DIR)
$(RCS_OUT): $(BUILD_DIR)/%.res: %.rc
	@echo compiling $<
	$(RC) $(RCFLAGS) $(DEFINES) $(INCLUDES) -O coff -o $@ -i $<

$(BUILD_DIR):
	@echo $@
	$(MKDIR) $(call normalize,$(sort $(BUILD_DIR)/ $(dir $(CXXS_OUT) $(dir $(RCS_OUT)))))

#$(SCINTILLA_TARGET):
#	$(MAKE) -C $(SCINTILLA_DIR)/win32

$(CONFIGS_OUT): $(TARGET_DIR)/%.xml: $(SOURCE_DIR)/%.xml
	@echo $@
	$(CP) $(call normalize,$< $@)

clean:
	-$(RM) $(call normalize,$(TARGET_BINARY) $(CONFIGS_OUT))
	-$(RMDIR) $(call normalize,$(BUILD_DIR))
#	-$(MAKE) -C $(SCINTILLA_DIR)/win32 $@

.PHONY: all clean

-include $(CXXS_OUT:%.o=%.d)
