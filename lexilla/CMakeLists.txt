cmake_minimum_required(VERSION 3.20)

project(Lexilla LANGUAGES CXX)

# Lexilla library sources
set(LEXILLA_SOURCES
    src/Lexilla.cxx
)

# Lexer sources (add major lexers - can expand later)
file(GLOB LEXER_SOURCES
    "lexers/Lex*.cxx"
)

# LexLib sources
file(GLOB LEXLIB_SOURCES
    "lexlib/*.cxx"
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lexlib
    ${CMAKE_SOURCE_DIR}/scintilla/include
)

# Create static library
add_library(lexilla STATIC
    ${LEXILLA_SOURCES}
    ${LEXLIB_SOURCES}
    ${LEXER_SOURCES}
)

# Compiler settings
target_compile_definitions(lexilla PRIVATE
    SCI_LEXER
)

if(APPLE)
    target_compile_options(lexilla PRIVATE
        -Wno-deprecated-declarations
    )
endif()

# Export include directories
target_include_directories(lexilla PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Installation
install(TARGETS lexilla
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
