cmake_minimum_required(VERSION 3.20)

project(Scintilla LANGUAGES CXX C OBJCXX)

# Scintilla source files (platform-independent)
set(SCINTILLA_SOURCES
    src/AutoComplete.cxx
    src/CallTip.cxx
    src/CaseConvert.cxx
    src/CaseFolder.cxx
    src/CellBuffer.cxx
    src/CharacterCategoryMap.cxx
    src/CharacterType.cxx
    src/CharClassify.cxx
    src/ContractionState.cxx
    src/ChangeHistory.cxx
    src/DBCS.cxx
    src/Decoration.cxx
    src/Document.cxx
    src/EditModel.cxx
    src/Editor.cxx
    src/EditView.cxx
    src/Indicator.cxx
    src/KeyMap.cxx
    src/LineMarker.cxx
    src/Geometry.cxx
    src/MarginView.cxx
    src/PerLine.cxx
    src/PositionCache.cxx
    src/RESearch.cxx
    src/RunStyles.cxx
    src/ScintillaBase.cxx
    src/Selection.cxx
    src/Style.cxx
    src/UniConversion.cxx
    src/UniqueString.cxx
    src/UndoHistory.cxx
    src/ViewStyle.cxx
    src/XPM.cxx
)

# Platform-specific sources
if(APPLE)
    set(SCINTILLA_PLATFORM_SOURCES
        cocoa/ScintillaCocoa.mm
        cocoa/ScintillaView.mm
        cocoa/InfoBar.mm
        cocoa/PlatCocoa.mm
    )
    
    # Cocoa-specific include directories
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/cocoa
    )
elseif(WIN32)
    set(SCINTILLA_PLATFORM_SOURCES
        win32/PlatWin.cxx
        win32/ScintillaWin.cxx
    )
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lexlib
)

# Create static library
add_library(scintilla STATIC
    ${SCINTILLA_SOURCES}
    ${SCINTILLA_PLATFORM_SOURCES}
)

# Platform-specific settings
if(APPLE)
    target_link_libraries(scintilla PUBLIC
        ${COCOA_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${APPKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
    )
    
    # Enable ARC (Automatic Reference Counting) for Objective-C++
    target_compile_options(scintilla PRIVATE
        -fobjc-arc
        -Wno-deprecated-declarations
    )
endif()

# Export include directories
target_include_directories(scintilla PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Installation
install(TARGETS scintilla
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
