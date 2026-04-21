cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows.Storage LANGUAGES C CXX)

add_library(${PROJECT_NAME} STATIC
    src/4J_Storage.cpp
    src/stdafx.cpp
    src/STO_DLC.cpp
    src/STO_Main.cpp
    src/STO_SaveGame.cpp
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
	OUTPUT_NAME "4J_Storage"
    ARCHIVE_OUTPUT_DIRECTORY "${4JLibs_SOURCE_DIR}/libs"
)