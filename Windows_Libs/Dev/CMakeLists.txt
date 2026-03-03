cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(Input)
add_subdirectory(Profile)
add_subdirectory(Render)
add_subdirectory(Storage)
