cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows64.Input LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    src/4J_Input.cpp
    src/INP_ForceFeedback.cpp
    src/INP_Keyboard.cpp
    src/INP_Main.cpp
    src/INP_StringCheck.cpp
    src/LinkedList.cpp
    src/stdafx.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    4JLibs.Globals
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    inc/stdafx.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	OUTPUT_NAME "4J_Input"
    ARCHIVE_OUTPUT_DIRECTORY "${4JLibs_SOURCE_DIR}/libs"
)