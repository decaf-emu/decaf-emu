cmake_minimum_required(VERSION 3.2)

project(decaf-emu C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE USING_CONAN)
if(USING_CONAN)
    conan_basic_setup(NO_OUTPUT_DIRS)
endif()

# Disable PCH by default on older versions of CMake
if(${CMAKE_VERSION} VERSION_LESS "3.16.0")
    set(DECAF_PCH_DEFAULT OFF)
    message(WARNING "You are using a CMake which does not support PCH (<3.16).  This will adversely affect compile times.")
else()
    set(DECAF_PCH_DEFAULT ON)
endif()

option(DECAF_FFMPEG          "Build with ffmpeg support"                    ON)
option(DECAF_VULKAN          "Build with Vulkan rendering support"          ON)
option(DECAF_QT              "Build with Qt support"                        ON)

option(DECAF_BUILD_TESTS     "Build tests"                                  OFF)
option(DECAF_BUILD_TOOLS     "Build tools"                                  OFF)

option(DECAF_GIT_VERSION     "Generate a version header from git state"     ON)
option(DECAF_JIT_PROFILING   "Build with JIT profiling support"             OFF)
option(DECAF_VALGRIND        "Build with Valgrind support"                  OFF)
option(DECAF_PCH             "Build with precompiled headers"               ${DECAF_PCH_DEFAULT})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)

# Setup install directories
include(GNUInstallDirs)
if(WIN32)
    set(DECAF_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}")
    set(DECAF_INSTALL_DOCSDIR "${CMAKE_INSTALL_PREFIX}")
    set(DECAF_INSTALL_RESOURCESDIR "${CMAKE_INSTALL_PREFIX}/resources")
else()
    set(DECAF_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
    set(DECAF_INSTALL_DOCSDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}")
    set(DECAF_INSTALL_RESOURCESDIR "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/resources")
endif()

if(DECAF_JIT_PROFILING)
    add_definitions(-DDECAF_JIT_ALLOW_PROFILING)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(XCB QUIET)
    find_package(X11 QUIET)
    find_package(WAYLAND QUIET)

    if(${XCB_FOUND})
        add_definitions(-DDECAF_PLATFORM_XCB)
        set(DECAF_PLATFORM_XCB TRUE)
    endif()

    if(${X11_FOUND})
        add_definitions(-DDECAF_PLATFORM_XLIB)
        set(DECAF_PLATFORM_XLIB TRUE)
    endif()

    if(${WAYLAND_FOUND})
        add_definitions(-DDECAF_PLATFORM_WAYLAND)
        set(DECAF_PLATFORM_WAYLAND TRUE)
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DDECAF_PLATFORM_COCOA)
    set(DECAF_PLATFORM_COCOA TRUE)
endif()

find_package(Threads REQUIRED)


if(VCPKG_TARGET_TRIPLET)
    find_package(c-ares CONFIG REQUIRED)
    find_package(CURL CONFIG REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(unofficial-libuv CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)

    set(CARES_LIBRARY c-ares::cares)
    set(CURL_LIBRARY CURL::libcurl)
    set(LIBUV_LIBRARY unofficial::libuv::libuv)
    set(OPENSSL_LIBRARY OpenSSL::SSL)
    set(SDL2_LIBRARY SDL2::SDL2)
    set(SDL2_MAIN_LIBRARY SDL2::SDL2main)
    set(ZLIB_LIBRARY ZLIB::ZLIB)
else()
    find_package(CARES REQUIRED)
    find_package(CURL REQUIRED)
    find_package(LibUV REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(ZLIB REQUIRED)

    set(CARES_LIBRARY CARES::CARES)
    set(CURL_LIBRARY CURL::libcurl)
    set(LIBUV_LIBRARY LibUV::LibUV)
    set(OPENSSL_LIBRARY OpenSSL::SSL)
    set(SDL2_LIBRARY SDL2::SDL2)
    set(SDL2_MAIN_LIBRARY SDL2::SDL2main)
    set(ZLIB_LIBRARY ZLIB::ZLIB)
endif()

if(DECAF_FFMPEG)
    find_package(FFMPEG REQUIRED)
    set(FFMPEG_LIBRARY FFMPEG::AVCODEC FFMPEG::AVFILTER FFMPEG::AVUTIL FFMPEG::SWSCALE)
    add_definitions(-DDECAF_FFMPEG)
endif()

# TODO: Remove this definitions as it is no longer optional
add_definitions(-DDECAF_SDL)

if(DECAF_VULKAN)
    find_package(Vulkan 1.1.106.0 REQUIRED) # Vulkan_INCLUDE_DIRS and Vulkan_LIBRARIES
    add_library(vulkan INTERFACE IMPORTED)
    set_target_properties(vulkan PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${Vulkan_INCLUDE_DIRS}
        INTERFACE_LINK_LIBRARIES ${Vulkan_LIBRARIES})
    add_definitions(-DVULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0)

    if(MSVC)
        add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    endif()

    if(DECAF_PLATFORM_XCB)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    endif()

    if(DECAF_PLATFORM_XLIB)
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    endif()

    if(DECAF_PLATFORM_WAYLAND)
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif()

    if(DECAF_PLATFORM_COCOA)
        add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
    endif()

    add_definitions(-DDECAF_VULKAN)
endif()

if(DECAF_VALGRIND)
    add_definitions(-DDECAF_VALGRIND)
endif()

if(DECAF_QT)
    find_package(Qt6 COMPONENTS Core Concurrent Widgets Svg SvgWidgets Xml)
    if(NOT Qt6_FOUND)
        find_package(Qt5 5.15 COMPONENTS Core Concurrent Widgets Svg Xml REQUIRED)
    endif()

    set(QT_DIR Qt${QT_VERSION_MAJOR}_DIR)
    add_definitions(-DDECAF_QT)
endif()

# Build third party libraries
add_subdirectory("libraries")

# Setup compile options
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /FS") # Parallel source builds
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GA") # Optimises TLS access
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4") # Lets be specific about warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd26812") # Allow unscoped enums
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4201") # Allow the use of unnamed structs/unions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # Allow unreferenced formal parameters
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4366") # Allow unaligned uint64_t (permitted on x64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4324") # Disable structure padding warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4297") # Disable nothrow warning, VulkanSDK engineers dont know how to code

    # Link time code generation
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

    add_definitions(-DNOMINMAX)
    add_definitions(-DUNICODE -D_UNICODE)

    # Disable warnings about using non-portable string function variants
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    # Disable warnings about using deprecated std::wstring_convert
    add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

    # Disable linker warnins about missing PDBs
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")

    # Set minimum windows version to enable newer APIs
    add_definitions(-D_WIN32_WINNT=0x0600 -DWINVER=0x0600)
else()
    add_definitions(-DDECAF_USE_STDLAYOUT_BITFIELD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")

    if(APPLE)
        add_definitions(-D_DARWIN_C_SOURCE)
    else()
        link_libraries(stdc++fs)
    endif()
endif()

# Macro to map filters to folder structure for MSVC projects
macro(GroupSources groupname curdir)
    if(MSVC)
        file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)

        foreach(child ${children})
            if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
                GroupSources(${rootname} ${groupname}/${child} ${curdir}/${child})
            else()
                string(REPLACE "/" "\\" safegroupname ${groupname})
                source_group(${safegroupname} FILES ${PROJECT_SOURCE_DIR}/${rootdir}${curdir}/${child})
            endif()
        endforeach()
    endif()
endmacro()

macro(AutoGroupPCHFiles)
    if(MSVC)
        source_group("CMake PCH" FILES
            "${PROJECT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/cmake_pch.hxx"
            "${PROJECT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/cmake_pch.cxx")
    endif()
endmacro()

if(DECAF_GIT_VERSION)
    # Generate build information
    include(GetGitRevisionDescription)

    function(get_timestamp _var)
        string(TIMESTAMP timestamp UTC)
        set(${_var} "${timestamp}" PARENT_SCOPE)
    endfunction()

    get_git_head_revision(GIT_REF_SPEC GIT_REV)
    git_describe(GIT_DESC --always --long --dirty)
    git_branch_name(GIT_BRANCH)
    get_timestamp(BUILD_DATE)

    set(BUILD_VERSION "0")
    if ($ENV{CI})
       if ($ENV{TRAVIS})
          set(BUILD_TAG $ENV{TRAVIS_TAG})
       elseif($ENV{APPVEYOR})
          set(BUILD_TAG $ENV{APPVEYOR_REPO_TAG_NAME})
       endif()

       if (BUILD_TAG)
          string(REGEX MATCH "${CMAKE_MATCH_1}-([0-9]+)" OUTVAR ${BUILD_TAG})
          if (${CMAKE_MATCH_COUNT} GREATER 0)
              set(BUILD_VERSION ${CMAKE_MATCH_1})
          endif()
       endif()
    endif()
else()
    set(GIT_REV "local")
    set(BUILD_NAME "decaf-emu")
    set(BUILD_VERSION "0")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/decaf_buildinfo.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/generated/decaf_buildinfo.h" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/generated")

add_subdirectory("src")
add_subdirectory("resources")

if(DECAF_BUILD_TOOLS)
    add_subdirectory("tools")
endif()

if(DECAF_BUILD_TESTS)
    enable_testing()
    add_subdirectory("tests")
endif()
