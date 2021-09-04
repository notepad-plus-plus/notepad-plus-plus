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
SOURCE_DIR := ../src
SOURCE_EXCLUDE := $(SOURCE_DIR)/tools/%
TARGET_DIR := ../bin
TARGET_BASE := NotepadPP
TARGET_SUFFIX := .exe
CONFIG_FILES := langs.model.xml stylers.model.xml shortcuts.xml contextMenu.xml
GCC_DIR := .
SCINTILLA_DIR := ../../scintilla

CXX := $(CROSS_COMPILE)g++
CXXFLAGS := -include $(GCC_DIR)/include/various.h -std=c++17 -Wno-conversion-null
RC := $(CROSS_COMPILE)windres
RCFLAGS :=
DEFINES := UNICODE _UNICODE _WIN32_WINNT=0x0600 TIXML_USE_STL TIXMLA_USE_STL
INCLUDES := $(SCINTILLA_DIR)/include
LD := $(CROSS_COMPILE)g++
LDFLAGS := -municode -mwindows
LIBRARIES := comctl32 crypt32 dbghelp ole32 sensapi shlwapi uuid uxtheme version wininet wintrust
SCINTILLA_TARGET := $(SCINTILLA_DIR)/bin/libscilexer.a
SCINTILLA_LIBRARIES := imm32 msimg32 oleaut32
SUBMAKEFLAGS := -O --no-print-directory

ifeq "$(strip $(DEBUG))" ""
CXXFLAGS += -O2 -Os
LDFLAGS += -s
DEFINES += NDEBUG
BUILD_TYPE := $(CROSS_COMPILE)release
else
CXXFLAGS += -g -Wall -Wpedantic -Wconversion-null
#DEFINES += DEBUG
BUILD_TYPE := $(CROSS_COMPILE)debug
endif

BUILD_DIR := $(GCC_DIR)/$(BUILD_TYPE)
TARGET_BINARY := $(TARGET_DIR)/$(TARGET_BASE)-$(BUILD_TYPE)$(TARGET_SUFFIX)

# Preparations:
ifeq "$(strip $(VERBOSE))" ""
AT := @
endif

ifeq "$(strip $(windir))" ""
# not a Windows system
MKDIR := mkdir -p
RMDIR := rm -rf
CP := cp
RM := rm -f
normalize-path = $1
else ifneq "$(wildcard $(dir $(SHELL))ls.exe)" ""
# a Windows system with UNIX-like shell
MKDIR := $(dir $(SHELL))mkdir.exe -p
RMDIR := $(dir $(SHELL))rm.exe -rf
CP := $(dir $(SHELL))cp.exe
RM := $(dir $(SHELL))rm.exe -f
normalize-path = $1
else
# a standard Windows system
MKDIR := mkdir
RMDIR := rmdir /q /s
CP := copy /y
RM := del /q
normalize-path = $(subst /,\,$1)
endif

list-subtree = $(foreach entry,$(wildcard $1/*),$(entry) $(call list-subtree,$(entry)))
GCC_DIR_TREE := $(patsubst $(GCC_DIR)/%,%,$(call list-subtree,$(GCC_DIR)))
SOURCE_DIR_TREE := $(patsubst $(SOURCE_DIR)/%,%,$(filter-out $(SOURCE_EXCLUDE),$(call list-subtree,$(SOURCE_DIR))))

INCLUDES += $(addprefix $(SOURCE_DIR)/,$(sort $(dir $(filter %.h %.hpp,$(SOURCE_DIR_TREE)))))

vpath %.cpp $(GCC_DIR) $(SOURCE_DIR)
CXX_TARGETS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(sort $(filter %.cpp,$(GCC_DIR_TREE) $(SOURCE_DIR_TREE))))

vpath %.rc $(GCC_DIR) $(SOURCE_DIR)
RC_TARGETS := $(patsubst %.rc,$(BUILD_DIR)/%.res,$(sort $(filter %.rc,$(GCC_DIR_TREE) $(SOURCE_DIR_TREE))))

CONFIG_TARGETS := $(addprefix $(TARGET_DIR)/,$(CONFIG_FILES))

# Actions:
.PHONY: .force all binary clean
.force:

GOALS := $(addprefix $(MAKELEVEL)-,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all))

ifneq "$(filter 0-all,$(GOALS))" ""
SUBMAKEFLAGS += $(if $(NUMBER_OF_PROCESSORS),-j$(NUMBER_OF_PROCESSORS),)
.NOTPARALLEL:
all: $(SCINTILLA_TARGET)
	$(AT)$(MAKE) $(SUBMAKEFLAGS) binary
else
all: binary
endif

$(SCINTILLA_TARGET): $(if $(filter 1-binary,$(GOALS)),,.force)
	$(AT)$(MAKE) $(SUBMAKEFLAGS) -C $(SCINTILLA_DIR)/win32 $(SCINTILLA_TARGET:$(SCINTILLA_DIR)/%=../%)

binary: $(TARGET_BINARY) $(CONFIG_TARGETS)

$(BUILD_DIR):
	@echo BUILD_DIR = $@
	$(AT)$(MKDIR) $(call normalize-path,$(sort $(BUILD_DIR)/ $(dir $(CXX_TARGETS) $(dir $(RC_TARGETS)))))

$(CXX_TARGETS): | $(BUILD_DIR)
$(CXX_TARGETS): $(BUILD_DIR)/%.o: %.cpp
	@echo compiling $<
	$(AT)$(CXX) $(CXXFLAGS) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) -MMD -c -o $@ $<

$(RC_TARGETS): | $(BUILD_DIR)
$(RC_TARGETS): $(BUILD_DIR)/%.res: %.rc
	@echo compiling $<
	$(AT)$(RC) $(RCFLAGS) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))  -O coff -o $@ -i $<

$(TARGET_BINARY): $(CXX_TARGETS) $(RC_TARGETS) $(SCINTILLA_TARGET)
	@echo linking $@
	$(AT)$(LD) $(LDFLAGS) $^ $(addprefix -l,$(LIBRARIES) $(SCINTILLA_LIBRARIES)) -static -o $@

$(CONFIG_TARGETS): $(TARGET_DIR)/%.xml: $(SOURCE_DIR)/%.xml
	@echo config $@
	$(AT)$(CP) $(call normalize-path,$< $@)

clean:
	-$(AT)$(RM) $(call normalize-path,$(TARGET_BINARY) $(CONFIG_TARGETS))
	-$(AT)$(RMDIR) $(call normalize-path,$(BUILD_DIR))
	-$(AT)$(MAKE) $(SUBMAKEFLAGS) -C $(SCINTILLA_DIR)/win32 $@
# the `clean` target of Scintilla leaves some artifacts, thus clean up after it
	-$(AT)$(RM) $(call normalize-path,$(SCINTILLA_DIR)/bin/*.a $(SCINTILLA_DIR)/bin/*.dll)

-include $(CXX_TARGETS:%.o=%.d)
