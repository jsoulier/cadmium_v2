#
# SPDX-License-Identifier: MIT
# Copyright (c) 2021-present Guillermo Trabes
# ARSLab - Carleton University
# Copyright (c) 2021-present jsoulier
# ARSLab - Carleton University
# Copyright (c) 2021-present Román Cárdenas Rodríguez
# ARSLab - Carleton University
#

cmake_minimum_required(VERSION 3.24)
project(cadmium)

add_subdirectory(json)
add_subdirectory(unordered_dense)

add_library(cadmium INTERFACE)
target_include_directories(cadmium INTERFACE include)
target_link_libraries(cadmium INTERFACE nlohmann_json unordered_dense)
set_target_properties(cadmium PROPERTIES CXX_STANDARD 17)
set_target_properties(cadmium PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if(GLM_ENABLE)
    target_link_libraries(cadmium INTERFACE glm::glm)
    target_compile_definitions(cadmium INTERFACE GLM_ENABLE_EXPERIMENTAL=1)
    target_compile_definitions(cadmium INTERFACE CADMIUM_GLM_ENABLE=1)
endif()
if(SPDLOG_ENABLE)
    target_link_libraries(cadmium INTERFACE spdlog::spdlog)
    target_compile_definitions(cadmium INTERFACE CADMIUM_SPDLOG_ENABLE=1)
endif()
if(TRACY_ENABLE)
    target_link_libraries(cadmium INTERFACE Tracy::TracyClient)
    target_compile_definitions(cadmium INTERFACE CADMIUM_TRACY_ENABLE=1)
endif()
