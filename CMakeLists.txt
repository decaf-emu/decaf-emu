cmake_minimum_required(VERSION 2.8.4)
include(ExternalProject)

project(decaf-emu C CXX)
option(DECAF_GLFW "Build with GLFW support" ON)
option(DECAF_SDL "Build with SDL support" ON)
option(DECAF_VALGRIND "Build with Valgrind support" OFF)

set(PUGIXML_DIR "libraries/pugixml")
include_directories("${PUGIXML_DIR}/src")
add_library(pugixml STATIC "${PUGIXML_DIR}/src/pugixml.cpp")
set(PUGIXML_LIBRARIES pugixml)

set(ADDRLIB_DIR "libraries/addrlib")
include_directories("${ADDRLIB_DIR}/include")
include_directories("${ADDRLIB_DIR}/src")
add_library(addrlib STATIC
    "${ADDRLIB_DIR}/src/addrinterface.cpp"
    "${ADDRLIB_DIR}/src/core/addrelemlib.cpp"
    "${ADDRLIB_DIR}/src/core/addrlib.cpp"
    "${ADDRLIB_DIR}/src/core/addrobject.cpp"
    "${ADDRLIB_DIR}/src/r600/r600addrlib.cpp"
    )
set(ADDRLIB_LIBRARIES addrlib)

set(EXCMD_DIR "libraries/excmd")
include_directories("${EXCMD_DIR}/src")

set(SPDLOG_DIR "libraries/spdlog")
include_directories("${SPDLOG_DIR}/include")

set(GSL_DIR "libraries/gsl-lite")
include_directories("${GSL_DIR}/include")

set(IMGUI_DIR "libraries/imgui")
include_directories("${IMGUI_DIR}")
add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    )
set(IMGUI_LIBRARIES imgui)

set(CEREAL_DIR "libraries/cereal")
include_directories("${CEREAL_DIR}/include")

set(FIXEDPOINT_DIR "libraries/fixed_point")
include_directories("${FIXEDPOINT_DIR}/include")

set(ASMJIT_DIR "libraries/asmjit")
include_directories("${ASMJIT_DIR}/src")
externalproject_add(asmjit
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ASMJIT_DIR}"
    CMAKE_CACHE_ARGS
        -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
        -DCMAKE_C_FLAGS:string=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS:string=${CMAKE_CXX_FLAGS}
        -DASMJIT_STATIC:string=on
    INSTALL_COMMAND ""
    )
externalproject_get_property(asmjit BINARY_DIR)
set(ASMJIT_LIBRARIES "${BINARY_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}asmjit.a")

set(GLBINDING_DIR "libraries/glbinding")
include_directories("${GLBINDING_DIR}/source/glbinding/include")
externalproject_add(glbinding
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${GLBINDING_DIR}"
    CMAKE_CACHE_ARGS
        -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
        -DCMAKE_C_FLAGS:string=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS:string=${CMAKE_CXX_FLAGS}
        -DBUILD_SHARED_LIBS:string=off
        -DOPTION_BUILD_TESTS:string=off
    INSTALL_COMMAND ""
    )
externalproject_get_property(glbinding BINARY_DIR)
set(GLBINDING_LIBRARIES "${BINARY_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}glbinding.a")
include_directories("${BINARY_DIR}/source/glbinding/include")

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})
add_definitions(-DDECAF_SDL)
add_definitions(-DDECAF_USE_STDLAYOUT_BITFIELD)
if (DECAF_VALGRIND)
    add_definitions(-DDECAF_VALGRIND)
endif()

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    if (APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()

add_subdirectory("src")
