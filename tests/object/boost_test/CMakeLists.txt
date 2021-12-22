cmake_minimum_required(VERSION 2.6)

project(boost_test)
set(CXX_FLAGS -g -Wall)
set(CMAKE_CXX_COMPLIER "g++")
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_executable(boost_test boost_test.cpp)