cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(4JLibs.Windows.Render LANGUAGES C CXX)

set(LIBPNG_SOURCES
    libpng/png.c
    libpng/pngerror.c
    libpng/pngget.c
    libpng/pngmem.c
    libpng/pngpread.c
    libpng/pngread.c
    libpng/pngrio.c
    libpng/pngrtran.c
    libpng/pngrutil.c
    libpng/pngset.c
    libpng/pngtest.c
    libpng/pngtrans.c
    libpng/pngwio.c
    libpng/pngwrite.c
    libpng/pngwtran.c
    libpng/pngwutil.c
)

add_library(${PROJECT_NAME} STATIC
    ${LIBPNG_SOURCES}
    4J_Render.cpp
    RendererCBuff.cpp
    RendererCore.cpp
    RendererMatrix.cpp
    RendererState.cpp
    RendererTexture.cpp
    RendererVertex.cpp
    stdafx.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/zlib
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    stdafx.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	OUTPUT_NAME "4J_Render"
)

set_source_files_properties(${LIBPNG_SOURCES} PROPERTIES
    LANGUAGE C
    SKIP_PRECOMPILE_HEADERS ON
)