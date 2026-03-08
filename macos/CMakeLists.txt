cmake_minimum_required(VERSION 3.20)
project(NotepadPlusPlusMac LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)

# Root of the Notepad++ source tree
set(NPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# ============================================================
# Win32 Shim Library
# ============================================================
# The shim include directory MUST come first so that
# #include <windows.h> resolves to our shim, not system headers.
set(SHIM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shim/include")

file(GLOB SHIM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shim/src/*.mm")

add_library(win32shim STATIC ${SHIM_SOURCES})
target_include_directories(win32shim
    PUBLIC "${SHIM_INCLUDE_DIR}"
)
target_compile_options(win32shim PRIVATE
    -Wno-deprecated-declarations
    -Wno-writable-strings
    -fobjc-arc
)
target_link_libraries(win32shim PUBLIC
    "-framework Foundation"
    "-framework AppKit"
    "-framework CoreGraphics"
    "-framework CoreText"
    "-framework CoreFoundation"
    "-framework Security"
)

# ============================================================
# Scintilla Library (Cocoa)
# ============================================================
set(SCINTILLA_DIR "${NPP_ROOT}/scintilla")
set(SCINTILLA_SRC_DIR "${SCINTILLA_DIR}/src")
set(SCINTILLA_COCOA_DIR "${SCINTILLA_DIR}/cocoa")
set(SCINTILLA_INCLUDE_DIR "${SCINTILLA_DIR}/include")

file(GLOB SCINTILLA_CORE_SOURCES "${SCINTILLA_SRC_DIR}/*.cxx")
file(GLOB SCINTILLA_COCOA_SOURCES "${SCINTILLA_COCOA_DIR}/*.mm")

add_library(Scintilla STATIC
    ${SCINTILLA_CORE_SOURCES}
    ${SCINTILLA_COCOA_SOURCES}
)
target_include_directories(Scintilla
    PUBLIC "${SCINTILLA_INCLUDE_DIR}"
    PRIVATE "${SCINTILLA_SRC_DIR}"
    PRIVATE "${SCINTILLA_COCOA_DIR}"
)
target_compile_definitions(Scintilla PRIVATE
    SCI_LEXER
    SCI_NAMESPACE
    MACOSX
)
target_compile_options(Scintilla PRIVATE
    -Wno-deprecated-declarations
    -Wno-unused-parameter
    -fobjc-arc
)
target_link_libraries(Scintilla PUBLIC
    "-framework Cocoa"
    "-framework QuartzCore"
)

# ============================================================
# Lexilla Library
# ============================================================
set(LEXILLA_DIR "${NPP_ROOT}/lexilla")
set(LEXILLA_SRC_DIR "${LEXILLA_DIR}/src")
set(LEXILLA_INCLUDE_DIR "${LEXILLA_DIR}/include")
set(LEXILLA_LEXLIB_DIR "${LEXILLA_DIR}/lexlib")
set(LEXILLA_LEXERS_DIR "${LEXILLA_DIR}/lexers")
set(LEXILLA_ACCESS_DIR "${LEXILLA_DIR}/access")

file(GLOB LEXILLA_LEXLIB_SOURCES "${LEXILLA_LEXLIB_DIR}/*.cxx")
file(GLOB LEXILLA_LEXER_SOURCES "${LEXILLA_LEXERS_DIR}/*.cxx")

add_library(Lexilla STATIC
    ${LEXILLA_LEXLIB_SOURCES}
    ${LEXILLA_LEXER_SOURCES}
    "${LEXILLA_SRC_DIR}/Lexilla.cxx"
)
target_include_directories(Lexilla
    PUBLIC "${LEXILLA_INCLUDE_DIR}"
    PUBLIC "${LEXILLA_ACCESS_DIR}"
    PRIVATE "${SHIM_INCLUDE_DIR}"     # For LexUser.cxx which includes <windows.h>
    PRIVATE "${LEXILLA_LEXLIB_DIR}"
    PRIVATE "${LEXILLA_LEXERS_DIR}"
    PRIVATE "${SCINTILLA_INCLUDE_DIR}"
)
target_compile_definitions(Lexilla PRIVATE
    LEXILLA_STATIC
)
target_compile_options(Lexilla PRIVATE
    -Wno-deprecated-declarations
    -Wno-unused-parameter
)

# ============================================================
# Vendored Libraries
# ============================================================

# pugixml
set(PUGIXML_DIR "${NPP_ROOT}/PowerEditor/src/pugixml")
add_library(pugixml STATIC "${PUGIXML_DIR}/pugixml.cpp")
target_include_directories(pugixml PUBLIC "${PUGIXML_DIR}")

# uchardet
set(UCHARDET_DIR "${NPP_ROOT}/PowerEditor/src/uchardet")
file(GLOB UCHARDET_SOURCES "${UCHARDET_DIR}/*.cpp")
add_library(uchardet STATIC ${UCHARDET_SOURCES})
target_include_directories(uchardet PUBLIC "${UCHARDET_DIR}")

# ============================================================
# Notepad++ PowerEditor Sources (Phase 0: non-GUI subset)
# ============================================================
set(NPP_SRC_DIR "${NPP_ROOT}/PowerEditor/src")

# For Phase 0, we compile a minimal subset to verify the shim works.
# These files are core logic that should compile with the shim headers.
# More files will be added in subsequent phases.

# Phase 0 target: just verify the shim + vendored libs compile
# and that key N++ headers can be parsed.
# Phase 0: Compile a simple test file that includes key N++ headers
# through the shim layer. Actual N++ source files will be added
# incrementally as we stub out more Windows-only headers.
add_library(npp_phase0_test STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/npp_headers_test.cpp"
    "${NPP_SRC_DIR}/MISC/sha2/sha-256.cpp"
    "${NPP_SRC_DIR}/Utf8_16.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/xmlMatchedTagsHighlighter.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/Buffer.cpp"
    "${NPP_SRC_DIR}/Parameters.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/FunctionCallTip.cpp"
    "${NPP_SRC_DIR}/EncodingMapper.cpp"
    "${NPP_SRC_DIR}/MISC/sha1/sha1.cpp"
    "${NPP_SRC_DIR}/MISC/sha1/calc_sha1.cpp"
    "${NPP_SRC_DIR}/MISC/sha512/sha512.cpp"
    "${NPP_SRC_DIR}/MISC/Common/FileInterface.cpp"
    "${NPP_SRC_DIR}/MISC/Common/verifySignedfile.cpp"
    "${NPP_SRC_DIR}/MISC/Common/Common.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/SmartHighlighter.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/ScintillaCtrls.cpp"
    "${NPP_SRC_DIR}/MISC/Common/SortLocale.cpp"
    "${NPP_SRC_DIR}/dpiManagerV2.cpp"
    "${NPP_SRC_DIR}/lastRecentFileList.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/ScintillaEditView.cpp"
    "${NPP_SRC_DIR}/Notepad_plus.cpp"
    "${NPP_SRC_DIR}/NppBigSwitch.cpp"
    "${NPP_SRC_DIR}/NppCommands.cpp"
    "${NPP_SRC_DIR}/NppNotification.cpp"
    "${NPP_SRC_DIR}/NppIO.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/AutoCompletion.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/columnEditor.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/DocTabView.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/FindReplaceDlg.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/GoToLineDlg.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/Printer.cpp"
    "${NPP_SRC_DIR}/ScintillaComponent/UserDefineDialog.cpp"
    "${NPP_SRC_DIR}/lesDlgs.cpp"
    "${NPP_SRC_DIR}/localization.cpp"
    "${NPP_SRC_DIR}/Notepad_plus_Window.cpp"
    "${NPP_SRC_DIR}/NppDarkMode.cpp"
    # "${NPP_SRC_DIR}/DarkMode/DarkMode.cpp"  # Excluded: uses PE/IAT hooking (Windows-only)
    "${NPP_SRC_DIR}/MISC/PluginsManager/PluginsManager.cpp"
    "${NPP_SRC_DIR}/MISC/md5/md5Dlgs.cpp"
)

# Add all WinControls source files
file(GLOB_RECURSE NPP_WINCONTROLS_SOURCES "${NPP_SRC_DIR}/WinControls/*.cpp")
# Exclude files that will be replaced with macOS-native implementations
list(FILTER NPP_WINCONTROLS_SOURCES EXCLUDE REGEX "ReadDirectoryChanges")
list(FILTER NPP_WINCONTROLS_SOURCES EXCLUDE REGEX "CustomFileDialog")  # Uses IFileDialog COM - replace with NSOpenPanel/NSSavePanel
list(FILTER NPP_WINCONTROLS_SOURCES EXCLUDE REGEX "pluginsAdmin")      # Uses WinInet - replace with NSURLSession
target_sources(npp_phase0_test PRIVATE ${NPP_WINCONTROLS_SOURCES})
# Gather all subdirectories under WinControls/ for includes
file(GLOB NPP_WINCONTROLS_DIRS "${NPP_SRC_DIR}/WinControls/*/")
file(GLOB NPP_MISC_DIRS "${NPP_SRC_DIR}/MISC/*/")

target_include_directories(npp_phase0_test
    PRIVATE "${SHIM_INCLUDE_DIR}"     # Must be first!
    PRIVATE "${NPP_SRC_DIR}"
    PRIVATE "${NPP_SRC_DIR}/MISC"
    PRIVATE "${NPP_SRC_DIR}/MISC/Common"
    PRIVATE "${NPP_SRC_DIR}/ScintillaComponent"
    PRIVATE "${NPP_SRC_DIR}/WinControls"
    PRIVATE ${NPP_WINCONTROLS_DIRS}
    PRIVATE "${NPP_SRC_DIR}/WinControls/StaticDialog/RunDlg"
    PRIVATE ${NPP_MISC_DIRS}
    PRIVATE "${NPP_SRC_DIR}/DarkMode"
    PRIVATE "${NPP_SRC_DIR}/json"
    PRIVATE "${SCINTILLA_INCLUDE_DIR}"
    PRIVATE "${LEXILLA_INCLUDE_DIR}"
    PRIVATE "${LEXILLA_ACCESS_DIR}"
    PRIVATE "${PUGIXML_DIR}"
    PRIVATE "${UCHARDET_DIR}"
)
target_compile_definitions(npp_phase0_test PRIVATE
    UNICODE
    _UNICODE
    __APPLE__
    NOMINMAX
    "NPP_SCINTILLA_VERSION=\"5.5.3\""
    "NPP_LEXILLA_VERSION=\"5.4.0\""
    "NPP_BOOST_REGEX_VERSION=\"1.90.0\""
)
target_compile_options(npp_phase0_test PRIVATE
    -Wno-deprecated-declarations
    -Wno-writable-strings
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -Wno-c++11-narrowing
    -include "${SHIM_INCLUDE_DIR}/windows.h"
)
target_link_libraries(npp_phase0_test
    win32shim
    pugixml
    uchardet
)

# ============================================================
# Phase 0 verification target
# ============================================================
# This executable just links everything together to verify compilation.
# It's not a runnable app yet - that comes in Phase 1.
add_executable(npp_build_test
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/build_test.mm"
)
target_include_directories(npp_build_test PRIVATE
    "${SHIM_INCLUDE_DIR}"
    "${SCINTILLA_INCLUDE_DIR}"
    "${LEXILLA_INCLUDE_DIR}"
)
target_link_libraries(npp_build_test
    win32shim
    Scintilla
    Lexilla
    pugixml
    uchardet
    "-framework Foundation"
    "-framework AppKit"
)
target_compile_options(npp_build_test PRIVATE -fobjc-arc)

# ============================================================
# Scintilla Bridge Library
# ============================================================
# This is compiled WITHOUT the Win32 shim force-include to avoid
# conflicts between ScintillaView.h's WM_COMMAND/WM_NOTIFY values
# and our shim's values. It provides a C interface to ScintillaView.
add_library(scintilla_bridge STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/scintilla_bridge.mm"
)
target_include_directories(scintilla_bridge
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/platform"
    PRIVATE "${SCINTILLA_COCOA_DIR}"
    PRIVATE "${SCINTILLA_INCLUDE_DIR}"
)
target_compile_options(scintilla_bridge PRIVATE
    -fobjc-arc
    -Wno-deprecated-declarations
)
target_link_libraries(scintilla_bridge PUBLIC
    Scintilla
)

# The win32shim needs scintilla_bridge.h include path and the bridge at link time
target_include_directories(win32shim
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/platform"
)
target_link_libraries(win32shim PUBLIC
    scintilla_bridge
)

# ============================================================
# Phase 1: Window + Scintilla App
# ============================================================
# A runnable macOS app with a window containing a ScintillaView editor.
# Demonstrates: NSApplication, window creation, Scintilla integration.
add_executable(npp_phase1_app
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/main.mm"
)
target_include_directories(npp_phase1_app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/platform"
    "${SHIM_INCLUDE_DIR}"
    "${SCINTILLA_INCLUDE_DIR}"
    "${LEXILLA_INCLUDE_DIR}"
)
target_link_libraries(npp_phase1_app
    win32shim
    scintilla_bridge
    Scintilla
    Lexilla
    pugixml
    uchardet
    "-framework Foundation"
    "-framework AppKit"
    "-framework QuartzCore"
)
target_compile_options(npp_phase1_app PRIVATE
    -fobjc-arc
    -Wno-deprecated-declarations
)

message(STATUS "=== Notepad++ macOS Port ===")
message(STATUS "Phase 0: Shim compilation test (npp_phase0_test)")
message(STATUS "Phase 1: Window + Scintilla app (npp_phase1_app)")
message(STATUS "Shim headers: ${SHIM_INCLUDE_DIR}")
message(STATUS "Scintilla: ${SCINTILLA_DIR}")
message(STATUS "Lexilla: ${LEXILLA_DIR}")
