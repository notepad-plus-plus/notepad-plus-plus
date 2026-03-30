cmake_minimum_required(VERSION 3.20)
project(NotepadPlusPlusMac LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)

# Root of the Notepad++ source tree
set(NPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# ============================================================
# Code Signing Configuration (opt-in)
# ============================================================
option(CODESIGN_ENABLED "Enable code signing for the app bundle and DMG" OFF)
set(CODESIGN_IDENTITY "Developer ID Application" CACHE STRING
    "Code signing identity (e.g., 'Developer ID Application: Your Name (TEAMID)')")
set(CODESIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/platform/MacNote.entitlements" CACHE FILEPATH
    "Path to entitlements plist for hardened runtime")

if(CODESIGN_ENABLED)
    message(STATUS "Code signing: ENABLED")
    message(STATUS "  Identity: ${CODESIGN_IDENTITY}")
    message(STATUS "  Entitlements: ${CODESIGN_ENTITLEMENTS}")
    if(NOT EXISTS "${CODESIGN_ENTITLEMENTS}")
        message(FATAL_ERROR "Entitlements file not found: ${CODESIGN_ENTITLEMENTS}")
    endif()
else()
    message(STATUS "Code signing: DISABLED (pass -DCODESIGN_ENABLED=ON to enable)")
endif()

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
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/shim/src"
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
    "-framework UniformTypeIdentifiers"
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
# Notepad++ PowerEditor Sources (compatibility compile subset)
# ============================================================
set(NPP_SRC_DIR "${NPP_ROOT}/PowerEditor/src")

# We compile a minimal subset to verify the shim works.
# These files are core logic that should compile with the shim headers.
# This target is for compilation verification only.

# Compile-test target: just verify the shim + vendored libs compile
# and that key N++ headers can be parsed.
# Compile a simple test file that includes key N++ headers
# through the shim layer. Actual N++ source files will be added
# incrementally as we stub out more Windows-only headers.
add_library(npp_macos_compile_test STATIC
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
target_sources(npp_macos_compile_test PRIVATE ${NPP_WINCONTROLS_SOURCES})
# Gather all subdirectories under WinControls/ for includes
file(GLOB NPP_WINCONTROLS_DIRS "${NPP_SRC_DIR}/WinControls/*/")
file(GLOB NPP_MISC_DIRS "${NPP_SRC_DIR}/MISC/*/")

target_include_directories(npp_macos_compile_test
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
target_compile_definitions(npp_macos_compile_test PRIVATE
    UNICODE
    _UNICODE
    __APPLE__
    NOMINMAX
    "NPP_SCINTILLA_VERSION=\"5.5.3\""
    "NPP_LEXILLA_VERSION=\"5.4.0\""
    "NPP_BOOST_REGEX_VERSION=\"1.90.0\""
)
target_compile_options(npp_macos_compile_test PRIVATE
    -Wno-deprecated-declarations
    -Wno-writable-strings
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -Wno-c++11-narrowing
    -include "${SHIM_INCLUDE_DIR}/windows.h"
)
target_link_libraries(npp_macos_compile_test
    win32shim
    pugixml
    uchardet
)

# ============================================================
# Compatibility verification target
# ============================================================
# This executable just links everything together to verify compilation.
# It's not the primary runnable app target.
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
# App icon generation (.icns from logo.png)
# ============================================================
set(ICONSET_DIR "${CMAKE_BINARY_DIR}/AppIcon.iconset")
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/AppIcon.icns"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ICONSET_DIR}"
    COMMAND sips -z 16 16     "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_16x16.png"
    COMMAND sips -z 32 32     "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_16x16@2x.png"
    COMMAND sips -z 32 32     "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_32x32.png"
    COMMAND sips -z 64 64     "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_32x32@2x.png"
    COMMAND sips -z 128 128   "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_128x128.png"
    COMMAND sips -z 256 256   "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_128x128@2x.png"
    COMMAND sips -z 256 256   "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_256x256.png"
    COMMAND sips -z 512 512   "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_256x256@2x.png"
    COMMAND sips -z 512 512   "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_512x512.png"
    COMMAND sips -z 583 583   "${NPP_ROOT}/logo.png" --out "${ICONSET_DIR}/icon_512x512@2x.png"
    COMMAND iconutil -c icns "${ICONSET_DIR}" -o "${CMAKE_BINARY_DIR}/AppIcon.icns"
    DEPENDS "${NPP_ROOT}/logo.png"
    COMMENT "Generating AppIcon.icns from logo.png"
)
add_custom_target(AppIcon DEPENDS "${CMAKE_BINARY_DIR}/AppIcon.icns")

# ============================================================
# Main macOS app target
# ============================================================
add_executable(MacOSNotePP
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/app_main.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/app_state.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/app_delegate.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/document_manager.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/file_operations.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/find_replace.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/edit_commands.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/split_view.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/session_manager.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/preferences_dialog.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/menu_builder.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/wndproc.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/appearance.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/scintilla_config.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/change_history.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/lexer_styles.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/language_defs.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/bookmarks.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/autocomplete.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/context_menu.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/status_bar.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/recent_files.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/settings_manager.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/file_monitor_mac.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/save_prompt.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/about_dialog.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/keyboard_shortcuts.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/brace_match.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/auto_indent.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/auto_close.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/sync_scroll.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/document_map.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/function_list_parser.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/function_list_panel.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/clipboard_history_panel.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/panel_divider.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/file_path_ops.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/tab_bar_view.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/tab_context_menu.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/smart_highlight.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/incremental_search.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/find_in_files.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/search_results_panel.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/toolbar.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/print_support.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/hash_tools.mm"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/macro_manager.mm"
)
target_include_directories(MacOSNotePP PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/platform"
    "${SHIM_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/shim/src"
    "${SCINTILLA_INCLUDE_DIR}"
    "${LEXILLA_INCLUDE_DIR}"
    "${UCHARDET_DIR}"
)
target_link_libraries(MacOSNotePP
    win32shim
    scintilla_bridge
    Scintilla
    Lexilla
    pugixml
    uchardet
    "-framework Foundation"
    "-framework AppKit"
    "-framework QuartzCore"
    "-framework UniformTypeIdentifiers"
    "-framework CoreServices"
)
target_compile_options(MacOSNotePP PRIVATE
    -fobjc-arc
    -Wno-deprecated-declarations
)
add_dependencies(MacOSNotePP AppIcon)
add_custom_command(TARGET MacOSNotePP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NPP_ROOT}/logo.png"
        "$<TARGET_FILE_DIR:MacOSNotePP>/logo.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/AppIcon.icns"
        "$<TARGET_FILE_DIR:MacOSNotePP>/AppIcon.icns"
    COMMENT "Copying logo.png and AppIcon.icns"
)

# ============================================================
# Packaging: App bundle, code signing, and DMG
# ============================================================
set(NPP_MACOS_DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set(NPP_MACOS_APP_BUNDLE "${NPP_MACOS_DIST_DIR}/MacNote++.app")
set(NPP_MACOS_APP_CONTENTS "${NPP_MACOS_APP_BUNDLE}/Contents")
set(NPP_MACOS_APP_MACOS "${NPP_MACOS_APP_CONTENTS}/MacOS")
set(NPP_MACOS_APP_RESOURCES "${NPP_MACOS_APP_CONTENTS}/Resources")
set(NPP_MACOS_DMG_ROOT "${NPP_MACOS_DIST_DIR}/dmg-root")
set(NPP_MACOS_DMG_APP_BUNDLE "${NPP_MACOS_DMG_ROOT}/MacNote++.app")
set(NPP_MACOS_UNSIGNED_DMG "${NPP_MACOS_DIST_DIR}/MacNote++-unsigned.dmg")
set(NPP_MACOS_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/MacNote++-Info.plist")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/MacNoteInfo.plist.in"
    "${NPP_MACOS_INFO_PLIST}"
    @ONLY
)

add_custom_target(MacOSNotePP_package
    DEPENDS MacOSNotePP
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_MACOS_APP_BUNDLE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NPP_MACOS_APP_MACOS}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NPP_MACOS_APP_RESOURCES}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:MacOSNotePP>"
        "${NPP_MACOS_APP_MACOS}/MacOSNotePP"
    COMMAND ditto --norsrc
        "${NPP_ROOT}/logo.png"
        "${NPP_MACOS_APP_MACOS}/logo.png"
    COMMAND ditto --norsrc
        "${NPP_ROOT}/logo.png"
        "${NPP_MACOS_APP_RESOURCES}/logo.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/AppIcon.icns"
        "${NPP_MACOS_APP_RESOURCES}/AppIcon.icns"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NPP_MACOS_INFO_PLIST}"
        "${NPP_MACOS_APP_CONTENTS}/Info.plist"
    COMMENT "Packaging MacNote++.app"
)

if(CODESIGN_ENABLED)
    # Sign and build DMG in a temp directory to avoid iCloud/FileProvider xattr
    # interference. iCloud-synced directories re-add xattrs that invalidate code
    # signatures, so all signing and DMG creation happens outside the sync zone.
    # NOTE: --deep is acceptable while the bundle has no nested frameworks/helpers.
    # Replace with explicit per-component signing if nested bundles are added.
    if(DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
        set(NPP_SIGN_STAGE_ROOT "$ENV{TMPDIR}")
    else()
        set(NPP_SIGN_STAGE_ROOT "/tmp")
    endif()
    set(NPP_SIGN_STAGE "${NPP_SIGN_STAGE_ROOT}/MacNotePP-sign-stage")
    set(NPP_SIGN_STAGE_APP "${NPP_SIGN_STAGE}/MacNote++.app")
    set(NPP_SIGN_STAGE_DMG_ROOT "${NPP_SIGN_STAGE}/dmg-root")

    add_custom_target(MacOSNotePP_sign
        DEPENDS MacOSNotePP_package
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_SIGN_STAGE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${NPP_SIGN_STAGE}"
        COMMAND ditto --norsrc
            "${NPP_MACOS_APP_BUNDLE}"
            "${NPP_SIGN_STAGE_APP}"
        COMMAND xattr -cr "${NPP_SIGN_STAGE_APP}"
        COMMAND codesign --force --options runtime
            --sign "${CODESIGN_IDENTITY}"
            --entitlements "${CODESIGN_ENTITLEMENTS}"
            --timestamp
            --deep
            "${NPP_SIGN_STAGE_APP}"
        COMMAND codesign --verify --deep --strict --verbose=2
            "${NPP_SIGN_STAGE_APP}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_MACOS_APP_BUNDLE}"
        COMMAND ditto "${NPP_SIGN_STAGE_APP}" "${NPP_MACOS_APP_BUNDLE}"
        COMMAND ${CMAKE_COMMAND} -E echo
            "Signed app bundle: ${NPP_MACOS_APP_BUNDLE}"
        COMMENT "Code signing MacNote++.app with '${CODESIGN_IDENTITY}'"
    )

    add_custom_target(MacOSNotePP_dmg
        DEPENDS MacOSNotePP_sign
        COMMAND ${CMAKE_COMMAND} -E make_directory "${NPP_SIGN_STAGE_DMG_ROOT}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_SIGN_STAGE_DMG_ROOT}/MacNote++.app"
        COMMAND ditto "${NPP_SIGN_STAGE_APP}" "${NPP_SIGN_STAGE_DMG_ROOT}/MacNote++.app"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_SIGN_STAGE_DMG_ROOT}/Applications"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "/Applications"
            "${NPP_SIGN_STAGE_DMG_ROOT}/Applications"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${NPP_SIGN_STAGE}/MacNote++.dmg"
        COMMAND hdiutil create
            -volname "MacNote++"
            -srcfolder "${NPP_SIGN_STAGE_DMG_ROOT}"
            -format UDZO
            -ov
            "${NPP_SIGN_STAGE}/MacNote++.dmg"
        COMMAND codesign --force --sign "${CODESIGN_IDENTITY}"
            --timestamp "${NPP_SIGN_STAGE}/MacNote++.dmg"
        COMMAND codesign --verify --verbose=2 "${NPP_SIGN_STAGE}/MacNote++.dmg"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${NPP_SIGN_STAGE}/MacNote++.dmg"
            "${NPP_MACOS_DIST_DIR}/MacNote++.dmg"
        COMMAND ${CMAKE_COMMAND} -E echo
            "Signed DMG: ${NPP_MACOS_DIST_DIR}/MacNote++.dmg"
        COMMAND ${CMAKE_COMMAND} -E echo
            "NOTE: If your project directory is iCloud-synced, signature verification"
        COMMAND ${CMAKE_COMMAND} -E echo
            "may fail locally due to Finder xattrs. The DMG is valid for distribution."
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_SIGN_STAGE}"
        COMMENT "Building and signing MacNote++.dmg"
    )
else()
    add_custom_target(MacOSNotePP_sign
        DEPENDS MacOSNotePP_package
        COMMENT "Code signing SKIPPED (CODESIGN_ENABLED=OFF)"
    )

    add_custom_target(MacOSNotePP_dmg
        DEPENDS MacOSNotePP_sign
        COMMAND ${CMAKE_COMMAND} -E make_directory "${NPP_MACOS_DMG_ROOT}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_MACOS_DMG_APP_BUNDLE}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${NPP_MACOS_APP_BUNDLE}"
            "${NPP_MACOS_DMG_APP_BUNDLE}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${NPP_MACOS_DMG_ROOT}/Applications"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "/Applications"
            "${NPP_MACOS_DMG_ROOT}/Applications"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${NPP_MACOS_UNSIGNED_DMG}"
        COMMAND hdiutil create
            -volname "MacNote++"
            -srcfolder "${NPP_MACOS_DMG_ROOT}"
            -format UDZO
            -ov
            "${NPP_MACOS_UNSIGNED_DMG}"
        COMMENT "Building MacNote++-unsigned.dmg"
    )
endif()

message(STATUS "=== Notepad++ macOS Port ===")
message(STATUS "Compatibility compile target: npp_macos_compile_test")
message(STATUS "App target: MacOSNotePP")
message(STATUS "Packaging target: MacOSNotePP_package")
message(STATUS "Signing target: MacOSNotePP_sign")
message(STATUS "DMG target: MacOSNotePP_dmg")
message(STATUS "Shim headers: ${SHIM_INCLUDE_DIR}")
message(STATUS "Scintilla: ${SCINTILLA_DIR}")
message(STATUS "Lexilla: ${LEXILLA_DIR}")
