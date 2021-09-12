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

# definitions

SCINTILLA_DIRECTORY := ../../scintilla
SCINTILLA_TARGET = libscilexer.a

BIN_DIRECTORY := ../bin
INSTALLER_DIRECTORY := ../installer
GCC_DIRECTORY := ../gcc
GCC_EXCLUDE := $(GCC_DIRECTORY)/gcc-%
SRC_DIRECTORY := ../src
SRC_EXCLUDE := $(SRC_DIRECTORY)/tools/%
TARGET_FILE := notepad++.exe

SRC_DATA := contextMenu.xml langs.model.xml shortcuts.xml stylers.model.xml
INSTALLER_DATA := autoCompletion/ functionList/ localization/ themes/

CXX := $(CROSS_COMPILE)g++
CXXFLAGS := -std=c++17 -Wno-conversion-null
RC := $(CROSS_COMPILE)windres
RCFLAGS :=
DEFINES := UNICODE _UNICODE _WIN32_WINNT=0x0600 TIXML_USE_STL TIXMLA_USE_STL
INCLUDES := $(SCINTILLA_DIRECTORY)/include
LD := $(CXX)
LDFLAGS := -municode -mwindows
LINK_DIRECTORIES :=
LINK_LIBRARIES := comctl32 crypt32 dbghelp ole32 sensapi shlwapi uuid uxtheme version wininet wintrust
LINK_LIBRARIES += imm32 msimg32 oleaut32
SUBMAKEFLAGS := -O --no-print-directory

# detect a request for a debug build
ifeq "$(filter-out 0,$(DEBUG))" ""
BUILD_TYPE := release
BUILD_SUFFIX :=
CXXFLAGS += -O2 -Os
LDFLAGS += -s
DEFINES += NDEBUG
else
BUILD_TYPE := debug
BUILD_SUFFIX := -debug
CXXFLAGS += -Og -g -Wall -Wpedantic -Wconversion-null
#DEFINES += DEBUG
endif

# preparations

# detect target CPU
TARGET_CPU := $(firstword $(subst -, ,$(shell $(CXX) -dumpmachine)))
ifeq "$(TARGET_CPU)" ""
$(error TARGET_CPU detection failed)
endif
ifneq "$(filter-out x86_64 i686,$(TARGET_CPU))" ""
$(error $(TARGET_CPU) build is unsupported)
endif
ifeq "$(TARGET_CPU)" "i686"
ifeq "$(wildcard $(windir)/system32/SensApi.dll)" ""
$(error $(TARGET_CPU) build requires "%windir%/system32/SensApi.dll" to be present)
endif
endif

TARGET_DIRECTORY := bin.$(TARGET_CPU)$(BUILD_SUFFIX)
TARGET_BINARY := $(TARGET_DIRECTORY)/$(TARGET_FILE)
BUILD_DIRECTORY := $(TARGET_DIRECTORY).build

SCINTILLA_TARGET := $(BUILD_DIRECTORY)/$(SCINTILLA_TARGET)

SRC_DATA := $(addprefix $(TARGET_DIRECTORY)/,$(SRC_DATA))
INSTALLER_DATA := $(addprefix $(TARGET_DIRECTORY)/,$(INSTALLER_DATA))

# detect a build outside of PowerEditor/gcc
MAKEFILE_DIRECTORY := $(patsubst %/,%,$(dir $(subst \,/,$(firstword $(MAKEFILE_LIST)))))
ifneq "$(MAKEFILE_DIRECTORY)" "."
PREFIX := $(patsubst %/,%,$(dir $(MAKEFILE_DIRECTORY)))

SCINTILLA_DIRECTORY := $(patsubst ../%,$(PREFIX)/%,$(SCINTILLA_DIRECTORY))

BIN_DIRECTORY := $(patsubst ../%,$(PREFIX)/%,$(BIN_DIRECTORY))
INSTALLER_DIRECTORY := $(patsubst ../%,$(PREFIX)/%,$(INSTALLER_DIRECTORY))
GCC_DIRECTORY := $(patsubst ../%,$(PREFIX)/%,$(GCC_DIRECTORY))
GCC_EXCLUDE := $(patsubst ../%,$(PREFIX)/%,$(GCC_EXCLUDE))
SRC_DIRECTORY := $(patsubst ../%,$(PREFIX)/%,$(SRC_DIRECTORY))
SRC_EXCLUDE := $(patsubst ../%,$(PREFIX)/%,$(SRC_EXCLUDE))

INCLUDES := $(patsubst ../%,$(PREFIX)/%,$(INCLUDES))
endif

CXXFLAGS += -include $(GCC_DIRECTORY)/gcc-fixes.h

# detect a request for verbose output
ifeq "$(filter-out 0,$(VERBOSE))" ""
AT := @
endif

# detect the current operating system
ifeq "$(windir)" ""
# not a Windows system
MKDIR := mkdir -p
CPDIR := cp -r
RMDIR := rm -rf
CP := cp
RM := rm -f
normalize-path = $1
else ifneq "$(wildcard $(dir $(SHELL))ls.exe)" ""
# a Windows system with a proper shell
MKDIR := $(dir $(SHELL))mkdir.exe -p
CPDIR := $(dir $(SHELL))cp.exe -r
RMDIR := $(dir $(SHELL))rm.exe -rf
CP := $(dir $(SHELL))cp.exe
RM := $(dir $(SHELL))rm.exe -f
normalize-path = $1
else
# a standard Windows system
MKDIR := mkdir
CPDIR := xcopy /q /e /i /y
RMDIR := rmdir /q /s
CP := copy /y
RM := del /q
normalize-path = $(subst /,\,$1)
endif

# discover files
list-subtree = $(foreach entry,$(wildcard $1/*),$(entry) $(call list-subtree,$(entry)))

GCC_SUBTREE := $(patsubst $(GCC_DIRECTORY)/%,%,$(filter-out $(GCC_EXCLUDE),$(call list-subtree,$(GCC_DIRECTORY))))
SRC_SUBTREE := $(patsubst $(SRC_DIRECTORY)/%,%,$(filter-out $(SRC_EXCLUDE),$(call list-subtree,$(SRC_DIRECTORY))))

INCLUDES += $(addprefix $(GCC_DIRECTORY)/,$(sort $(patsubst %/,%,$(dir $(filter %.h %.hpp,$(GCC_SUBTREE))))))
INCLUDES += $(addprefix $(SRC_DIRECTORY)/,$(sort $(patsubst %/,%,$(dir $(filter %.h %.hpp,$(SRC_SUBTREE))))))

vpath %.cpp $(GCC_DIRECTORY) $(SRC_DIRECTORY)
CXX_TARGETS := $(patsubst %.cpp,$(BUILD_DIRECTORY)/%.o,$(sort $(filter %.cpp,$(GCC_SUBTREE) $(SRC_SUBTREE))))

vpath %.rc $(GCC_DIRECTORY) $(SRC_DIRECTORY)
RC_TARGETS := $(patsubst %.rc,$(BUILD_DIRECTORY)/%.res,$(sort $(filter %.rc,$(GCC_SUBTREE) $(SRC_SUBTREE))))

# actions

GOALS := $(addprefix $(MAKELEVEL)-,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all))

.PHONY: .force all binary data clean fullclean
.force:

ifneq "$(filter 0-all,$(GOALS))" ""
SUBMAKEFLAGS += $(if $(NUMBER_OF_PROCESSORS),-j$(NUMBER_OF_PROCESSORS),)
.NOTPARALLEL:
all: $(SCINTILLA_TARGET)
	$(AT)$(MAKE) -f $(firstword $(MAKEFILE_LIST)) $(SUBMAKEFLAGS) binary
else
all: binary
endif

$(BUILD_DIRECTORY):
	@echo * creating BUILD_DIRECTORY $@
	$(AT)$(MKDIR) $(call normalize-path,$(sort $@ $(patsubst %/,%,$(dir $(CXX_TARGETS) $(RC_TARGETS)))))

$(SCINTILLA_TARGET): $(if $(filter 1-binary,$(GOALS)),,.force) | $(BUILD_DIRECTORY)
	$(AT)$(MAKE) $(SUBMAKEFLAGS) -C $(SCINTILLA_DIRECTORY)/win32 LIBLEX=$(CURDIR)/$(SCINTILLA_TARGET) $(CURDIR)/$(SCINTILLA_TARGET)

binary: $(TARGET_BINARY) data
	@echo *** $(TARGET_CPU) $(BUILD_TYPE) : $(CURDIR)/$(TARGET_BINARY) ***

$(CXX_TARGETS): $(BUILD_DIRECTORY)/%.o: %.cpp | $(BUILD_DIRECTORY)
	@echo * compiling $<
	$(AT)$(CXX) $(CXXFLAGS) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) -MMD -c -o $@ $<

$(RC_TARGETS): $(BUILD_DIRECTORY)/%.res: %.rc | $(BUILD_DIRECTORY)
	@echo * compiling $<
	$(AT)$(RC) $(RCFLAGS) $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES)) -O coff -o $@ -i $<

ifeq "$(TARGET_CPU)" "i686"
LINK_DIRECTORIES += $(BUILD_DIRECTORY)
$(BUILD_DIRECTORY)/libsensapi.a: | $(BUILD_DIRECTORY)
	@echo * generating $@
	$(AT)gendef $(call normalize-path,$(windir)/system32/SensApi.dll)
	$(AT)dlltool -d SensApi.def -k -l $(call normalize-path,$@)
	$(AT)$(RM) SensApi.def
endif

$(TARGET_DIRECTORY):
	@echo * creating TARGET_DIRECTORY $@
	$(AT)$(CPDIR) $(call normalize-path,$(BIN_DIRECTORY) $@)

$(TARGET_BINARY): $(CXX_TARGETS) $(RC_TARGETS) $(SCINTILLA_TARGET) | $(TARGET_DIRECTORY) $(if $(filter i686,$(TARGET_CPU)),$(BUILD_DIRECTORY)/libsensapi.a,)
	@echo * linking $@
	$(AT)$(LD) $(LDFLAGS) $^ $(addprefix -L,$(LINK_DIRECTORIES)) $(addprefix -l,$(LINK_LIBRARIES)) -static -o $@

data: $(patsubst %/,%,$(SRC_DATA) $(INSTALLER_DATA))

$(patsubst %/,%,$(filter %/,$(SRC_DATA))): $(TARGET_DIRECTORY)/%: $(SRC_DIRECTORY)/% | $(TARGET_DIRECTORY)
	@echo * copying $@
	$(AT)$(CPDIR) $(call normalize-path,$< $@)
$(filter-out %/,$(SRC_DATA)): $(TARGET_DIRECTORY)/%: $(SRC_DIRECTORY)/% | $(TARGET_DIRECTORY)
	@echo * copying $@
	$(AT)$(CP) $(call normalize-path,$< $@)

$(TARGET_DIRECTORY)/autoCompletion: $(INSTALLER_DIRECTORY)/APIs
$(TARGET_DIRECTORY)/functionList: $(INSTALLER_DIRECTORY)/functionList
$(TARGET_DIRECTORY)/localization: $(INSTALLER_DIRECTORY)/nativeLang
$(TARGET_DIRECTORY)/themes: $(INSTALLER_DIRECTORY)/themes
$(patsubst %/,%,$(filter %/,$(INSTALLER_DATA))): | $(TARGET_DIRECTORY)
	@echo * copying $@
	$(AT)$(CPDIR) $(call normalize-path,$< $@)
$(filter-out %/,$(INSTALLER_DATA)): | $(TARGET_DIRECTORY)
	@echo * copying $@
	$(AT)$(CP) $(call normalize-path,$< $@)

clean:
	-$(AT)$(RMDIR) $(call normalize-path,$(BUILD_DIRECTORY))
	-$(AT)$(MAKE) $(SUBMAKEFLAGS) -C $(SCINTILLA_DIRECTORY)/win32 $@

fullclean: clean
	-$(AT)$(RMDIR) $(call normalize-path,$(TARGET_DIRECTORY))

-include $(CXX_TARGETS:%.o=%.d)
