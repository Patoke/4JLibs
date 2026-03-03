cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows.Profile LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    4J_Profile.cpp
    PRO_AwardManager.cpp
    PRO_Data.cpp
    PRO_Main.cpp
    PRO_RichPresence.cpp
    PRO_Sys.cpp
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
	OUTPUT_NAME "4J_Profile"
)