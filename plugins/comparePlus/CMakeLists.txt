cmake_minimum_required (VERSION 3.5)

set (CMAKE_SYSTEM_NAME Windows)

option (WIN64				"Win 64-bit build"						ON)
option (MULTITHREAD			"Multithread change detection"			OFF)
option (OTHER_MOVED_ICONS	"Use alternative moves lines icons"		OFF)

# On Cmake invokation if debug logging is desired set the value of DLOG to include the bit-flag of each desired
# function to log:
# bit 0 - log compare algorithm
# bit 1 - log views synchronization
# bit 2 - log Notepad++ and Scintilla notifications
# bit 3 - log navigation commands diff visiting
set (DLOG	0	CACHE STRING	"Flags for debug log messages")

if (UNIX OR MINGW)
	set (CMAKE_SYSROOT "/usr")

	set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)
	set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
	set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)

	if (WIN64)
		set (toolchain_prefix   x86_64-w64-mingw32)
	else ()
		set (toolchain_prefix   i686-w64-mingw32)
	endif ()

	set (CMAKE_C_COMPILER   ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-gcc)
	set (CMAKE_CXX_COMPILER ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-g++)
	set (CMAKE_RC_COMPILER  ${CMAKE_SYSROOT}/bin/${toolchain_prefix}-windres)

	set (win32_inc_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/include)
	set (win32_lib_dir ${CMAKE_SYSROOT}/${toolchain_prefix}/lib)
endif ()

project (ComparePlus)

if (UNIX OR MINGW)
	set (defs
		-DUNICODE -D_UNICODE -DMINGW_HAS_SECURE_API=1 -D_WIN32 -DWIN32
		-D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM -DUNIX -DBOOST_REGEX_STANDALONE
	)

	if (NOT DEBUG)
		set (defs ${defs} -DNDEBUG)
	endif ()

	if (WIN64)
		set (defs ${defs} -DWIN64)
		message ("Win64 build is ON")
	endif ()

	if (MULTITHREAD)
		set (defs ${defs} -DMULTITHREAD)
		message ("Multithread change detection is ON")
	endif ()

	if (DLOG)
		set (defs ${defs} -DDLOG=${DLOG})
		message ("Debug log flags value: ${DLOG}")
	endif ()

	if (OTHER_MOVED_ICONS)
		set (defs ${defs} -DOTHER_MOVED_ICONS)
		message ("Using alternative moved lines icons.")
	endif ()

	set (CMAKE_CXX_FLAGS
		"-std=c++14 -O3 -static-libgcc -static-libstdc++ -Wall -Wno-unknown-pragmas"
	)

	set (CMAKE_MODULE_LINKER_FLAGS
		"-s"
	)

	set (CMAKE_SHARED_MODULE_PREFIX "")
else ()
	set (defs
		-DUNICODE -D_UNICODE -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES -D_WIN32 -DWIN32
		-D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOCOMM -DBOOST_REGEX_STANDALONE -DMULTITHREAD
	)

	set (CMAKE_CXX_FLAGS
		"/EHsc /LD /MT /MP /W4"
	)
endif ()

set (project_rc_files
	src/Compare.rc
)

set (project_sources
	src/NppAPI/DockingFeature/StaticDialog.cpp
	src/AboutDlg/URLCtrl.cpp
	src/AboutDlg/AboutDialog.cpp
	src/SettingsDlg/ColorCombo.cpp
	src/SettingsDlg/SettingsDialog.cpp
	src/CompareOptionsDlg/CompareOptionsDialog.cpp
	src/VisualFiltersDlg/VisualFiltersDialog.cpp
	src/NavDlg/NavDialog.cpp
	src/ProgressDlg/ProgressDlg.cpp
	src/Engine/Engine.cpp
	src/Tools.cpp
	src/UserSettings.cpp
	src/Compare.cpp
	src/LibGit2/LibGit2Helper.cpp
	src/NppHelpers.cpp
	src/LibHelpers.cpp
	src/SQLite/SqliteHelper.cpp
	src/Strings.cpp
)

include_directories (
	${win32_inc_dir}
	src/
	src/NppAPI/
	src/NppAPI/DockingFeature/
	src/Icons/
	src/Engine/
	src/AboutDlg/
	src/SettingsDlg/
	src/CompareOptionsDlg/
	src/VisualFiltersDlg/
	src/NavDlg/
	src/ProgressDlg/
	src/SQLite/
	src/LibGit2/
	src/boost_regex/include/
	src/nlohmann/
)

add_definitions (${defs})

add_library (ComparePlus MODULE ${project_rc_files} ${project_sources})

if (UNIX OR MINGW)
	find_library (comctl32
		NAMES libcomctl32.a
		PATHS ${win32_lib_dir}
	)

	find_library (comdlg32
		NAMES libcomdlg32.a
		PATHS ${win32_lib_dir}
	)

	find_library (shlwapi
		NAMES libshlwapi.a
		PATHS ${win32_lib_dir}
	)

	find_library (msimg32
		NAMES libmsimg32.a
		PATHS ${win32_lib_dir}
	)

	find_library (uxtheme
		NAMES libuxtheme.a
		PATHS ${win32_lib_dir}
	)

	target_link_libraries (ComparePlus ${comctl32} ${comdlg32} ${shlwapi} ${msimg32} ${uxtheme})

	set (INSTALL_PATH
		"$ENV{HOME}/wine/drive_c/Program Files/Notepad++/plugins/ComparePlus"
	)

	install (FILES ${CMAKE_BINARY_DIR}/ComparePlus.dll
		DESTINATION "${INSTALL_PATH}"
	)
else ()
	target_link_libraries (ComparePlus comctl32 comdlg32 shlwapi msimg32 uxtheme)

	set (INSTALL_PATH
		"${PROJECT_SOURCE_DIR}/Notepad++/plugins/ComparePlus"
	)

	install (TARGETS ComparePlus
		DESTINATION "${INSTALL_PATH}"
	)
endif ()

message ("Install destination: ${INSTALL_PATH}")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch")
		install (DIRECTORY libs/arm64/
			DESTINATION "${INSTALL_PATH}/libs"
		)
	else()
		install (DIRECTORY libs/x64/
			DESTINATION "${INSTALL_PATH}/libs"
		)
	endif()
else()
	install (DIRECTORY libs/x86/
		DESTINATION "${INSTALL_PATH}/libs"
	)
endif()

install (DIRECTORY res/localization/
	DESTINATION "${INSTALL_PATH}/localization"
)
