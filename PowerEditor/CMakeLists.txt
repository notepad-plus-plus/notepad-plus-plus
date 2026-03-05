cmake_minimum_required(VERSION 3.20)

project(PowerEditor LANGUAGES CXX C OBJCXX)

# Note: This is a minimal CMakeLists.txt to get the build system started
# The actual implementation will require creating macOS-specific source files
# For now, this creates a placeholder target

# Platform-specific main entry point
if(APPLE)
    set(MAIN_SOURCE
        src/main_mac.mm
    )
    
    set(COCOA_SOURCES
        src/cocoa/AppDelegate.mm
        src/cocoa/NotepadPlusWindowController.mm
    )
    
    set(COCOA_HEADERS
        src/cocoa/AppDelegate.h
        src/cocoa/NotepadPlusWindowController.h
    )
elseif(WIN32)
    set(MAIN_SOURCE
        src/winmain.cpp
    )
endif()

# Core source files (platform-independent logic)
# Note: Many of these will need platform abstraction
set(CORE_SOURCES
    src/Parameters.cpp
    src/Utf8_16.cpp
    src/EncodingMapper.cpp
    # TODO: Add more core sources as they are ported
)

# Platform abstraction layer
if(APPLE)
    set(PLATFORM_SOURCES
        src/platform/WindowManager_mac.mm
        src/platform/FileSystem_mac.mm
    )
    
    set(PLATFORM_HEADERS
        src/platform/PlatformTypes.h
        src/platform/WindowManager.h
        src/platform/FileSystem.h
    )
endif()

# Scintilla component sources
set(SCINTILLA_COMPONENT_SOURCES
    src/ScintillaComponent/Buffer.cpp
    src/ScintillaComponent/ScintillaEditView.cpp
    # TODO: Add more as ported
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MISC/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ScintillaComponent
    ${CMAKE_SOURCE_DIR}/scintilla/include
    ${CMAKE_SOURCE_DIR}/scintilla/cocoa
    ${CMAKE_SOURCE_DIR}/lexilla/include
)

# For now, create a minimal executable target
# This will be expanded as we port more code
if(APPLE)
    # Create macOS app bundle
    add_executable(notepadpp MACOSX_BUNDLE
        # Main entry point
        ${MAIN_SOURCE}
        # Cocoa UI
        ${COCOA_SOURCES}
        ${COCOA_HEADERS}
        # Platform abstraction layer
        ${PLATFORM_SOURCES}
        ${PLATFORM_HEADERS}
        # Core sources (commented out for now - will add as ported)
        # ${CORE_SOURCES}
        # ${SCINTILLA_COMPONENT_SOURCES}
    )
    
    # Link frameworks
    target_link_libraries(notepadpp
        scintilla
        lexilla
        # boostregex  # Not needed yet
        ${COCOA_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${APPKIT_FRAMEWORK}
        ${CORE_SERVICES_FRAMEWORK}
    )
    if(UNIFORM_TYPE_IDENTIFIERS_FRAMEWORK)
        target_link_libraries(notepadpp ${UNIFORM_TYPE_IDENTIFIERS_FRAMEWORK})
    endif()
    
    # Set bundle properties
    set_target_properties(notepadpp PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "Notepad++"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.notepad-plus-plus.notepad-plus-plus"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${NotepadPlusPlus_VERSION}
    )
    
elseif(WIN32)
    # Windows executable (for reference/comparison)
    add_executable(notepadpp WIN32
        ${MAIN_SOURCE}
        ${CORE_SOURCES}
        ${SCINTILLA_COMPONENT_SOURCES}
    )
    
    target_link_libraries(notepadpp
        scintilla
        lexilla
        boostregex
    )
endif()

# Installation
if(APPLE)
    install(TARGETS notepadpp
        BUNDLE DESTINATION .
    )
endif()
