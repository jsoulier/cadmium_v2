cmake_minimum_required(VERSION 3.1)
project(cadmium)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_library(cadmium INTERFACE)
target_include_directories(cadmium INTERFACE include/ json/include)

