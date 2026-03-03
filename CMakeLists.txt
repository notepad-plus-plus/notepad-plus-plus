cmake_minimum_required(VERSION 3.20)

# Project definition
project(NotepadPlusPlus
    VERSION 8.6.0
    DESCRIPTION "Notepad++ for macOS"
    LANGUAGES CXX C OBJCXX
)

option(NPP_BUILD_BOOSTREGEX "Build bundled boostregex static library" ON)
if(APPLE)
    # Current macOS port path does not link boostregex yet.
    # Keep it optional so core app build is not blocked.
    set(NPP_BUILD_BOOSTREGEX OFF CACHE BOOL "Build bundled boostregex static library" FORCE)
endif()

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOS specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build for Apple Silicon")
    
    # Enable Objective-C++ for Cocoa integration
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    
    # macOS frameworks
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(CORE_SERVICES_FRAMEWORK CoreServices REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
endif()

# Platform detection
if(WIN32)
    add_definitions(-D_WIN32 -DWIN32)
elseif(APPLE)
    add_definitions(-D__APPLE__ -DMACOS)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/PowerEditor/src
    ${CMAKE_SOURCE_DIR}/scintilla/include
    ${CMAKE_SOURCE_DIR}/lexilla/include
    ${CMAKE_SOURCE_DIR}/boostregex
)

# Compiler flags
if(APPLE)
    add_compile_options(
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -fvisibility=hidden
    )
    
    # Optimize for Apple Silicon
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        add_compile_options(-mcpu=apple-m1)
    endif()
endif()

# Subdirectories
add_subdirectory(scintilla)
add_subdirectory(lexilla)
if(NPP_BUILD_BOOSTREGEX)
    add_subdirectory(boostregex)
endif()
add_subdirectory(PowerEditor)

# Installation rules for macOS app bundle
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "Notepad++")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.notepad-plus-plus.notepad-plus-plus")
    set(MACOSX_BUNDLE_ICON_FILE "notepad++.icns")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2023 Notepad++ Team")
    
    # Install app bundle
    install(TARGETS notepadpp
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Notepad++ Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
if(APPLE)
    message(STATUS "  macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    message(STATUS "  Architecture: ${CMAKE_OSX_ARCHITECTURES}")
endif()
message(STATUS "")
