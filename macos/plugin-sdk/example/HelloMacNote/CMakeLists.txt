cmake_minimum_required(VERSION 3.20)
project(HelloMacNote LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_library(HelloMacNote MODULE HelloMacNote.cpp)

target_include_directories(HelloMacNote PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

set_target_properties(HelloMacNote PROPERTIES
    PREFIX ""
    SUFFIX ".dylib"
    OUTPUT_NAME "HelloMacNote"
)
