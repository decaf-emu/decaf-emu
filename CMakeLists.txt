cmake_minimum_required(VERSION 3.2)

project(decaf-emu C CXX)
option(DECAF_VALGRIND "Build with Valgrind support" OFF)
option(DECAF_SDL "Build with SDL support" ON)
option(DECAF_JIT_ALLOW_PROFILING "Build with JIT profiling support" ON)
option(DECAF_BUILD_TESTS "Build tests" OFF)
option(DECAF_BUILD_WUT_TESTS "Build tests which rely on wut" OFF)

set(DEVKITPPC $ENV{DEVKITPPC} CACHE STRING "Path to devkitPPC install")
set(WUT_ROOT $ENV{WUT_ROOT} CACHE STRING "Path to wut install")

if(DECAF_BUILD_WUT_TESTS AND (NOT DEVKITPPC OR NOT WUT_ROOT))
    message(FATAL_ERROR "You must have defined DEVKITPPC and WUT_ROOT to build wut tests.")
endif()

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)

if(DECAF_SDL)
    add_definitions(-DDECAF_SDL)
endif()

if(DECAF_VALGRIND)
    add_definitions(-DDECAF_VALGRIND)
endif()

if(DECAF_JIT_ALLOW_PROFILING)
    add_definitions(-DDECAF_JIT_ALLOW_PROFILING)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")

    add_definitions(-DNOMINMAX)
    add_definitions(-DUNICODE -D_UNICODE)
else()
    add_definitions(-DDECAF_USE_STDLAYOUT_BITFIELD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

# Macro to map filters to folder structure for MSVC projects
macro(GroupSources curdir)
    if(MSVC)
        file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)

        foreach(child ${children})
            if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
                GroupSources(${curdir}/${child})
            else()
                string(REPLACE "/" "\\" groupname ${curdir})
                source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
            endif()
        endforeach()
    endif()
endmacro()

add_subdirectory("libraries")
add_subdirectory("src")
add_subdirectory("tools")
add_subdirectory("resources")

if(DECAF_BUILD_TESTS OR DECAF_BUILD_WUT_TESTS)
    add_subdirectory("tests")
endif()
