cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs LANGUAGES CXX)

add_library(4JLibs.Globals INTERFACE)

target_include_directories(4JLibs.Globals INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

set(CMAKE_DEBUG_POSTFIX "_d")

# msvc specific flags
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(WIN32)
    add_subdirectory(impls/Windows_Libs)
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()
