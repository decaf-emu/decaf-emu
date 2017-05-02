cmake_minimum_required(VERSION 3.2)

project(decaf-emu C CXX)
option(DECAF_BUILD_TESTS "Build tests" OFF)
option(DECAF_BUILD_WUT_TESTS "Build tests which rely on wut" OFF)
option(DECAF_JIT_ALLOW_PROFILING "Build with JIT profiling support" ON)
option(DECAF_GL "Build with OpenGL rendering support" ON)
option(DECAF_SDL "Build with SDL support" ON)
option(DECAF_VALGRIND "Build with Valgrind support" OFF)
option(DECAF_VULKAN "Build with Vulkan rendering support" OFF)
option(DECAF_QT "Build with Qt support" OFF)

set(DEVKITPPC $ENV{DEVKITPPC} CACHE STRING "Path to devkitPPC install")
set(WUT_ROOT $ENV{WUT_ROOT} CACHE STRING "Path to wut install")

if(DECAF_BUILD_WUT_TESTS AND (NOT DEVKITPPC OR NOT WUT_ROOT))
    message(FATAL_ERROR "You must have defined DEVKITPPC and WUT_ROOT to build wut tests.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)

find_package(Threads REQUIRED)

if(DECAF_JIT_ALLOW_PROFILING)
    add_definitions(-DDECAF_JIT_ALLOW_PROFILING)
endif()

if(DECAF_GL)
    add_definitions(-DDECAF_GL)
endif()

if(DECAF_SDL)
    find_package(OpenGL REQUIRED)
    add_definitions(-DDECAF_SDL)
endif()

if(DECAF_VULKAN)
    find_package(Vulkan REQUIRED)
    add_definitions(-DDECAF_VULKAN)
endif()

if(DECAF_VALGRIND)
    add_definitions(-DDECAF_VALGRIND)
endif()

if(DECAF_QT)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt5Widgets)
    add_definitions(-DDECAF_QT)
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
    enable_testing()
    add_subdirectory("tests")
endif()
