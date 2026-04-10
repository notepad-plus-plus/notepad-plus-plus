# MacNote++ Plugin SDK
# For building plugins as .dylib files that load into MacNote++.
#
# Plugins include PluginInterface.h which pulls in Notepad_plus_msgs.h and Scintilla.h.
# On macOS, <windows.h> resolves to the Win32 shim from macos/shim/include/.

cmake_minimum_required(VERSION 3.20)
project(MacNotePlusPluginSDK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# Paths to headers
set(SHIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shim/include")
set(NPP_PLUGINS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../PowerEditor/src/MISC/PluginsManager")
set(SCINTILLA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../scintilla/include")

# Example plugin
add_subdirectory(example/HelloMacNote)

# Apply SDK include paths to all plugin targets
target_include_directories(HelloMacNote PRIVATE
    "${SHIM_DIR}"
    "${NPP_PLUGINS_DIR}"
    "${SCINTILLA_DIR}"
)
