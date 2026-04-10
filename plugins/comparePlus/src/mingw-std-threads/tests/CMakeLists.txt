project(stdthreadtest)
cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11 -Wall -Wextra)
add_executable(${PROJECT_NAME} tests.cpp)


