# addrlib
add_library(addrlib STATIC
    "addrlib/src/addrinterface.cpp"
    "addrlib/src/core/addrelemlib.cpp"
    "addrlib/src/core/addrlib.cpp"
    "addrlib/src/core/addrobject.cpp"
    "addrlib/src/r600/r600addrlib.cpp")
set_target_properties(addrlib PROPERTIES FOLDER libraries)
target_include_directories(addrlib
   PRIVATE "addrlib/src"
   PUBLIC "addrlib/include")

# libbinrec
set(BINREC_ENABLE_RTL_DEBUG_OPTIMIZE FALSE CACHE BOOL "Enable debug output from optimization passes")
set(BINREC_ENABLE_ASSERT FALSE CACHE BOOL "Enable basic assertion checks")
add_subdirectory(libbinrec)
set_target_properties(binrec PROPERTIES FOLDER libraries)

# catch
add_library(catch2 INTERFACE IMPORTED GLOBAL)
set_target_properties(catch2 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/catch/single_include/catch2")

# cereal
add_library(cereal INTERFACE IMPORTED GLOBAL)
set_target_properties(cereal PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cereal/include")

# cnl
add_library(cnl INTERFACE IMPORTED GLOBAL)
set_target_properties(cnl PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cnl/include")

# cpptoml
add_library(cpptoml INTERFACE IMPORTED GLOBAL)
set_target_properties(cpptoml PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "CPPTOML_USE_MAP"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cpptoml/include")

# excmd
add_library(excmd INTERFACE IMPORTED GLOBAL)
set_target_properties(excmd PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/excmd/src")

# fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER libraries)

# glslang
if(DECAF_VULKAN OR DECAF_BUILD_TOOLS)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "glslang: BUILD_SHARED_LIBS" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "glslang: BUILD_TESTING" FORCE)
    set(ENABLE_HLSL OFF CACHE BOOL "glslang: ENABLE_HLSL" FORCE)
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "glslang: ENABLE_GLSLANG_BINARIES" FORCE)
    set(ENABLE_OPT ON CACHE BOOL "glslang: ENABLE_OPT" FORCE)
    set(ENABLE_SPVREMAPPER OFF CACHE BOOL "glslang: ENABLE_SPVREMAPPER" FORCE)
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "glslang: SKIP_GLSLANG_INSTALL" FORCE)
    add_subdirectory(glslang)

    macro(remove_flag_from_target _target _flag)
        get_target_property(_target_cxx_flags ${_target} COMPILE_OPTIONS)
        if(_target_cxx_flags)
            list(REMOVE_ITEM _target_cxx_flags ${_flag})
            set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS "${_target_cxx_flags}")
        endif()
    endmacro()

    # As we inherit spv::Builder in libgpu, we must remove -fno-rtti
    remove_flag_from_target(SPIRV -fno-rtti)
endif()

# gsl
add_library(gsl INTERFACE IMPORTED GLOBAL)
set_target_properties(gsl PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "GSL_THROWS_FOR_TESTING"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/gsl-lite/include")

# imgui
add_library(imgui STATIC
    "imgui/imgui.cpp"
    "imgui/imgui_draw.cpp")
set_target_properties(imgui PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    FOLDER libraries)

# ovsocket
add_library(ovsocket INTERFACE IMPORTED GLOBAL)
set_target_properties(ovsocket PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/ovsocket/ovsocket")

# peglib
add_library(peglib INTERFACE IMPORTED GLOBAL)
set_target_properties(peglib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cpp-peglib")

# pugixml
add_library(pugixml STATIC
    "pugixml/src/pugixml.cpp")
set_target_properties(pugixml PROPERTIES
    FOLDER libraries
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src")

# spdlog
add_subdirectory(spdlog)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library instead of bundled" FORCE)
set_target_properties(spdlog PROPERTIES FOLDER libraries)

# Qt Advanced Docking System
if(DECAF_QT)
    set(BUILD_STATIC TRUE CACHE BOOL "ADS: Build the static library")
    set(BUILD_EXAMPLES FALSE CACHE BOOL "ADS: Build the examples")
    add_subdirectory(qtads EXCLUDE_FROM_ALL)
    set_target_properties(qtadvanceddocking PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/qtads/src"
        INTERFACE_COMPILE_DEFINITIONS "ADS_STATIC"
        FOLDER libraries)
endif()
