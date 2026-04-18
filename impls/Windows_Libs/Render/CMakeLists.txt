cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows.Render LANGUAGES C CXX)

set(LIBPNG_SOURCES
    vendor/libpng/png.c
    vendor/libpng/pngerror.c
    vendor/libpng/pngget.c
    vendor/libpng/pngmem.c
    vendor/libpng/pngpread.c
    vendor/libpng/pngread.c
    vendor/libpng/pngrio.c
    vendor/libpng/pngrtran.c
    vendor/libpng/pngrutil.c
    vendor/libpng/pngset.c
    vendor/libpng/pngtest.c
    vendor/libpng/pngtrans.c
    vendor/libpng/pngwio.c
    vendor/libpng/pngwrite.c
    vendor/libpng/pngwtran.c
    vendor/libpng/pngwutil.c
)

add_library(${PROJECT_NAME} STATIC
    ${LIBPNG_SOURCES}
    src/4J_Render.cpp
    src/RendererCBuff.cpp
    src/RendererCore.cpp
    src/RendererMatrix.cpp
    src/RendererState.cpp
    src/RendererTexture.cpp
    src/RendererVertex.cpp
    src/stdafx.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    4JLibs.Globals
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/zlib
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    inc/stdafx.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	OUTPUT_NAME "4J_Render"
    ARCHIVE_OUTPUT_DIRECTORY "${4JLibs_SOURCE_DIR}/libs"
)

set_source_files_properties(${LIBPNG_SOURCES} PROPERTIES
    LANGUAGE C
    SKIP_PRECOMPILE_HEADERS ON
)