cmake_minimum_required(VERSION 3.2)

project(decaf-emu C CXX)
option(DECAF_VALGRIND "Build with Valgrind support" OFF)
option(DECAF_SDL "Build with SDL support" ON)

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
