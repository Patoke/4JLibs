cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows.Input LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    4J_Input.cpp
    INP_ForceFeedback.cpp
    INP_Keyboard.cpp
    INP_Main.cpp
    INP_StringCheck.cpp
    LinkedList.cpp
    stdafx.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	OUTPUT_NAME "4J_Input"
)