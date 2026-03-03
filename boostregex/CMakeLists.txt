cmake_minimum_required(VERSION 3.20)

project(BoostRegex LANGUAGES CXX)

# Boost regex sources
set(BOOSTREGEX_SOURCES
    BoostRegExSearch.cxx
    UTF8DocumentIterator.cxx
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/boost
    ${CMAKE_SOURCE_DIR}/scintilla/include
    ${CMAKE_SOURCE_DIR}/scintilla/src
)

# Create static library
add_library(boostregex STATIC
    ${BOOSTREGEX_SOURCES}
)

# Compiler settings
if(APPLE)
    target_compile_options(boostregex PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-variable
    )
endif()

# Export include directories
target_include_directories(boostregex PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/boost
    ${CMAKE_SOURCE_DIR}/scintilla/src
)

# Installation
install(TARGETS boostregex
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
