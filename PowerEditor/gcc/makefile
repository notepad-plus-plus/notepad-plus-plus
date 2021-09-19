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

#
# definitions
#

ORIGIN_DIRECTORY = ..
# will be properly defined later
BUILD_DIRECTORY = .
TARGET_DIRECTORY = .

SOURCE_DATA =
SOURCE_IGNORE =
SOURCE_DATA += $(ORIGIN_DIRECTORY)/gcc=np
SOURCE_IGNORE += $(ORIGIN_DIRECTORY)/gcc/gcc-%
SOURCE_DATA += $(ORIGIN_DIRECTORY)/src=np
SOURCE_IGNORE += $(ORIGIN_DIRECTORY)/src/tools/%
SOURCE_DATA += $(ORIGIN_DIRECTORY)/../scintilla/include=sc
SOURCE_DATA += $(ORIGIN_DIRECTORY)/../scintilla/lexers=sc
SOURCE_DATA += $(ORIGIN_DIRECTORY)/../scintilla/lexlib=sc
SOURCE_DATA += $(ORIGIN_DIRECTORY)/../scintilla/src=sc
SOURCE_DATA += $(ORIGIN_DIRECTORY)/../scintilla/win32=sc
SOURCE_IGNORE += $(ORIGIN_DIRECTORY)/../scintilla/win32/%.rc
SOURCE_IGNORE += $(ORIGIN_DIRECTORY)/../scintilla/win32/ScintillaDLL.cxx
SOURCE_DATA += $(ORIGIN_DIRECTORY)/../boostregex=sc
SOURCE_IGNORE += $(ORIGIN_DIRECTORY)/../boostregex/boost/%

TARGET_BINARY = $(TARGET_DIRECTORY)/notepad++.exe
TARGET_DATA =
TARGET_DATA += $(TARGET_DIRECTORY)/contextMenu.xml=$(ORIGIN_DIRECTORY)/src/contextMenu.xml
TARGET_DATA += $(TARGET_DIRECTORY)/langs.model.xml=$(ORIGIN_DIRECTORY)/src/langs.model.xml
TARGET_DATA += $(TARGET_DIRECTORY)/shortcuts.xml=$(ORIGIN_DIRECTORY)/src/shortcuts.xml
TARGET_DATA += $(TARGET_DIRECTORY)/stylers.model.xml=$(ORIGIN_DIRECTORY)/src/stylers.model.xml
TARGET_DATA += $(TARGET_DIRECTORY)/change.log=$(ORIGIN_DIRECTORY)/bin/change.log
TARGET_DATA += $(TARGET_DIRECTORY)/doLocalConf.xml=$(ORIGIN_DIRECTORY)/bin/doLocalConf.xml
TARGET_DATA += $(TARGET_DIRECTORY)/readme.txt=$(ORIGIN_DIRECTORY)/bin/readme.txt
TARGET_DATA += $(TARGET_DIRECTORY)/userDefineLangs=$(ORIGIN_DIRECTORY)/bin/userDefineLangs
TARGET_DATA += $(TARGET_DIRECTORY)/autoCompletion=$(ORIGIN_DIRECTORY)/installer/APIs
TARGET_DATA += $(TARGET_DIRECTORY)/functionList=$(ORIGIN_DIRECTORY)/installer/functionList
TARGET_DATA += $(TARGET_DIRECTORY)/localization=$(ORIGIN_DIRECTORY)/installer/nativeLang
TARGET_DATA += $(TARGET_DIRECTORY)/themes=$(ORIGIN_DIRECTORY)/installer/themes

CXX = $(CROSS_COMPILE)g++
CXXFLAGS = -std=c++17
CXXFLAGS.np = -include $(ORIGIN_DIRECTORY)/gcc/gcc-fixes.h
CXXFLAGS.sc = -Wall -Wpedantic
RC = $(CROSS_COMPILE)windres
RCFLAGS =
CPP_PATH =
CPP_DEFINE =
CPP_DEFINE.np = UNICODE _UNICODE _WIN32_WINNT=0x0600 TIXML_USE_STL TIXMLA_USE_STL
CPP_DEFINE.sc = SCI_LEXER SCI_OWNREGEX
LD = $(CXX)
LDFLAGS = -municode -mwindows
LD_PATH = $(BUILD_DIRECTORY)
LD_LINK = comctl32 crypt32 dbghelp ole32 sensapi shlwapi uuid uxtheme version wininet wintrust
# for Scintilla
LD_LINK += imm32 msimg32 oleaut32
SUBMAKEFLAGS = -O --no-print-directory

# differentiate between release and debug builds
ifeq "$(filter-out 0,$(DEBUG))" ""
BUILD_TYPE = release
BUILD_SUFFIX =
CXXFLAGS += -O2 -Os
CXXFLAGS.np += -Wno-conversion-null
CPP_DEFINE += NDEBUG
LDFLAGS += -s
else
BUILD_TYPE = debug
BUILD_SUFFIX = -debug
CXXFLAGS += -g
CXXFLAGS.np += -Wall -Wpedantic
#CPP_DEFINE += DEBUG
CPP_DEFINE.sc += DEBUG
endif

#
# preparations
#

# detect compiler version
CXX_VERSION := $(shell $(CXX) -dumpversion)
ifeq "$(CXX_VERSION)" ""
$(error CXX_VERSION detection failed)
endif

# detect target CPU of the compiler
TARGET_CPU := $(firstword $(subst -, ,$(shell $(CXX) -dumpmachine)))
ifeq "$(TARGET_CPU)" ""
$(error TARGET_CPU detection failed)
endif
ifneq "$(filter-out x86_64 i686,$(TARGET_CPU))" ""
$(error $(TARGET_CPU) build is unsupported)
endif
ifeq "$(TARGET_CPU)" "i686"
# for some reason i686 versions of MinGW-w64 GCC don't include a linking library for SensApi.dll
# thus it is generated on the fly, but first check if the DLL is available
ifeq "$(wildcard $(windir)/system32/SensApi.dll)" ""
  $(error $(TARGET_CPU) build requires "%windir%/system32/SensApi.dll" to be present)
endif
# detect explicit definition of TARGET_CPU via command line to force a 32-bit build
ifeq "$(origin TARGET_CPU)" "command line"
  CXX += -m32
  RC += -Fpe-i386
endif
endif

# current build environment configuration
BUILD_CONFIGURATION = $(CXX)/$(CXX_VERSION)/$(TARGET_CPU)/$(BUILD_TYPE)

# properly define target and build directories
BUILD_DIRECTORY = bin.$(TARGET_CPU)$(BUILD_SUFFIX).build
TARGET_DIRECTORY = bin.$(TARGET_CPU)$(BUILD_SUFFIX)

# detect a build outside of PowerEditor/gcc
MAKEFILE_DIRECTORY = $(dir $(subst \,/,$(firstword $(MAKEFILE_LIST))))
ifneq "$(MAKEFILE_DIRECTORY)" "./"
ORIGIN_DIRECTORY = $(patsubst %/,%,$(dir $(MAKEFILE_DIRECTORY:%/=%)))
endif

# detect a request for a verbose output
ifeq "$(filter-out 0,$(VERBOSE))" ""
AT = @
endif

# detect the current operating system
ifeq "$(windir)" ""
# not a Windows system
MKDIR = mkdir -p
CPDIR = cp -r
RMDIR = rm -rf
CP = cp
RM = rm -f
normalize-path = $1
else ifneq "$(wildcard $(dir $(SHELL))ls.exe)" ""
# a Windows system with a proper shell
MKDIR = $(dir $(SHELL))mkdir.exe -p
CPDIR = $(dir $(SHELL))cp.exe -r
RMDIR = $(dir $(SHELL))rm.exe -rf
CP = $(dir $(SHELL))cp.exe
RM = $(dir $(SHELL))rm.exe -f
normalize-path = $1
else
# a standard Windows system
MKDIR = mkdir
CPDIR = xcopy /q /e /i /y
RMDIR = rmdir /q /s
CP = copy /y
RM = del /q
normalize-path = $(subst /,\,$1)
endif

# enable parallel job execution for recursive invocations
ifeq "$(filter -j%,$(MAKEFLAGS))" ""
ifneq "$(NUMBER_OF_PROCESSORS)" ""
  MAKEFLAGS += -j$(NUMBER_OF_PROCESSORS)
endif
endif

# functions for discovering files and directories to work on
data-keys = $(foreach i,$1,$(word 1,$(subst =, ,$i)))
data-value = $(word 2,$(subst =, ,$(filter $2=%,$1)))
# source-subtree; $1 = subtree root; $2 = ignore patterns; $3 = filter patterns;
source-subtree = \
$(foreach _item_,$(filter %/ $3,$(filter-out $2,$(wildcard $1/*/))),\
	$(if $(filter %/,$(_item_)),$(call source-subtree,$(_item_:%/=%),$2,$3),$(_item_))\
)
# source-items; $1 = filter patterns; [$2 = replace pattern;]
source-items = \
$(foreach _path_,$(call data-keys,$(SOURCE_DATA)),\
	$(eval _token_ = $(call data-value,$(SOURCE_DATA),$(_path_)))\
	$(eval _alias_ = $(_token_).$(notdir $(_path_)))\
	$(eval _tree_ = $(call source-subtree,$(_path_),$(filter $(_path_)/%,$(SOURCE_IGNORE)),$1))\
	$(if $(_tree_),\
		$(if $2,\
			$(eval $(subst %,$(_alias_)~%,$2) : CXXFLAGS += $$(CXXFLAGS.$(_token_)))\
			$(eval $(subst %,$(_alias_)~%,$2) : CPP_DEFINE += $$(CPP_DEFINE.$(_token_)))\
			$(foreach _type_,$1,\
				$(foreach _file_,$(filter $(_type_),$(_tree_)),\
					$(eval _item_ := $(patsubst $(_type_),$2,$(_alias_)~$(subst /,.,$(_file_:$(_path_)/%=%))))\
					$(_item_)\
					$(eval $(_item_) : $(_file_))\
					$(eval undefine _item_)\
				)\
			)\
			,\
			$(_tree_)\
		)\
	,)\
	$(eval undefine _token_)\
	$(eval undefine _alias_)\
	$(eval undefine _tree_)\
)
# target-items;
target-items = \
$(foreach _path_,$(call data-keys,$(TARGET_DATA)),\
	$(eval _source_ = $(call data-value,$(TARGET_DATA),$(_path_)))\
	$(_path_)\
	$(eval $(_path_) : $(_source_))\
)

#
# actions
#

.PHONY: all build data clean fullclean

all:
	$(AT)$(MAKE) -f $(firstword $(MAKEFILE_LIST)) $(SUBMAKEFLAGS) build

# skip expensive operations when their results won't be used
ifneq "$(filter-out all %clean,$(MAKECMDGOALS))" ""

build: $(TARGET_BINARY) data
	@echo *** $(subst /, / ,$(BUILD_CONFIGURATION)) : $(abspath $(TARGET_BINARY)) ***

CXX_ITEMS := $(call source-items,%.cpp %.cxx,$(BUILD_DIRECTORY)/%.o)
RC_ITEMS := $(call source-items,%.rc,$(BUILD_DIRECTORY)/%.res)

$(TARGET_BINARY): $(sort $(CXX_ITEMS) $(RC_ITEMS)) | $(TARGET_DIRECTORY)
	@echo * linking $@
	$(AT)$(LD) $(LDFLAGS) $(filter-out %.a,$^) $(addprefix -L,$(LD_PATH)) $(addprefix -l,$(LD_LINK)) -static -o $@

$(CXX_ITEMS): | $(BUILD_DIRECTORY) $(BUILD_DIRECTORY)/CPP_PATH
	@echo * compiling $(<:$(ORIGIN_DIRECTORY)/%=%)
	$(AT)$(CXX) $(CXXFLAGS) @$(BUILD_DIRECTORY)/CPP_PATH $(addprefix -D,$(CPP_DEFINE)) -MMD -c -o $@ $<

$(RC_ITEMS): | $(BUILD_DIRECTORY) $(BUILD_DIRECTORY)/CPP_PATH
	@echo * compiling $(<:$(ORIGIN_DIRECTORY)/%=%)
	$(AT)$(RC) $(RCFLAGS) @$(BUILD_DIRECTORY)/CPP_PATH $(addprefix -D,$(CPP_DEFINE)) -O coff -o $@ -i $<

CPP_PATH += $(sort $(patsubst %/,%,$(dir $(call source-items,%.h %.hpp))))

$(BUILD_DIRECTORY)/CPP_PATH: | $(BUILD_DIRECTORY)
	@echo * exporting $(notdir $@)
	$(file >$@,$(addprefix -I,$(CPP_PATH)))

TARGET_ITEMS := $(call target-items)

data: $(TARGET_ITEMS)

$(TARGET_ITEMS): | $(TARGET_DIRECTORY)
	@echo * copying $@
	$(AT)$(if $(wildcard $</*),$(CPDIR),$(CP)) $(call normalize-path,$< $@)

$(BUILD_DIRECTORY):
	@echo * creating BUILD_DIRECTORY $@
	$(AT)$(MKDIR) $(call normalize-path,$@)

$(TARGET_DIRECTORY):
	@echo * creating TARGET_DIRECTORY $@
	$(AT)$(MKDIR) $(call normalize-path,$@)

ifeq "$(TARGET_CPU)" "i686"
$(TARGET_BINARY): $(BUILD_DIRECTORY)/libsensapi.a
$(BUILD_DIRECTORY)/libsensapi.a: | $(BUILD_DIRECTORY)
	@echo * generating $@
	$(AT)gendef $(call normalize-path,$(firstword $(wildcard $(windir)/syswow64/SensApi.dll $(windir)/system32/SensApi.dll)))
	$(AT)dlltool -mi386 -f--32 -d SensApi.def -k -l $(call normalize-path,$@)
	$(AT)$(RM) SensApi.def
endif

-include $(CXX_ITEMS:%.o=%.d)

endif

clean:
	-$(AT)$(RMDIR) $(call normalize-path,$(BUILD_DIRECTORY))

fullclean: clean
	-$(AT)$(RMDIR) $(call normalize-path,$(TARGET_DIRECTORY))
