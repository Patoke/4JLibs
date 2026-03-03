cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows.Storage LANGUAGES C CXX)

add_library(${PROJECT_NAME} STATIC
    4J_Storage.cpp
    stdafx.cpp
    STO_DLC.cpp
    STO_Main.cpp
    STO_SaveGame.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	OUTPUT_NAME "4J_Storage"
)