cmake_minimum_required(VERSION 2.6)

project(object_oriented)

set(CXX_FLAGS -g -Wall)
set(CMAKE_CXX_COMPLIER "g++")
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_executable(objectoriented_test ObjectOriented_test.cpp Thread.cpp)
target_link_libraries(objectoriented_test pthread)
