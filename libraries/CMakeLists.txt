include(ExternalProject)

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

# glad
if(DECAF_GL)
    if(DECAF_GLAD_DEBUG)
        set(GLAD_DIR "glad-debug")
    else()
        set(GLAD_DIR "glad")
    endif()

    set(GLAD_SOURCE_FILES "${GLAD_DIR}/src/glad.c")

    if(WIN32)
        set(GLAD_SOURCE_FILES ${GLAD_SOURCE_FILES} "${GLAD_DIR}/src/glad_wgl.c")
    endif()

    if(DECAF_PLATFORM_XLIB)
        set(GLAD_SOURCE_FILES ${GLAD_SOURCE_FILES} "${GLAD_DIR}/src/glad_glx.c")
    endif()

    add_library(glad STATIC ${GLAD_SOURCE_FILES})
    set_target_properties(glad PROPERTIES FOLDER libraries)
    target_include_directories(glad PUBLIC "${GLAD_DIR}/include")

    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set_target_properties(glad PROPERTIES INTERFACE_LINK_LIBRARIES dl)
    endif()
endif()

# glslang
if(DECAF_VULKAN OR DECAF_BUILD_TOOLS)
    set(GLSLANG_DIR "glslang")
    externalproject_add(glslang
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${GLSLANG_DIR}"
        CMAKE_CACHE_ARGS
            -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS:string=${CMAKE_C_FLAGS}
            -DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}
            -DCMAKE_CXX_FLAGS:string=${CMAKE_CXX_FLAGS}
            -DBUILD_SHARED_LIBS:string=off
            -DENABLE_HLSL:string=off
            -DENABLE_GLSLANG_BINARIES:string=off
            -DENABLE_OPT:string=on
            -DENABLE_SPVREMAPPER:string=off
            -DSKIP_GLSLANG_INSTALL:string=on
        INSTALL_COMMAND ""
        )
    externalproject_get_property(glslang BINARY_DIR)
    externalproject_add_step(glslang
        copy_to_binary_spirv
        DEPENDEES build
        DEPENDERS install
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/SPIRV/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
    externalproject_add_step(glslang
        copy_to_binary_glslang
        DEPENDEES build
        DEPENDERS install
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/glslang/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")

    if (MSVC)
        externalproject_add_step(glslang
            copy_to_binary_glslang_osdependent
            DEPENDEES build
            DEPENDERS install
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/glslang/OSDependent/Windows/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
    else ()
        externalproject_add_step(glslang
            copy_to_binary_glslang_osdependent
            DEPENDEES build
            DEPENDERS install
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/glslang/OSDependent/Unix/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
    endif ()

    set_target_properties(glslang PROPERTIES FOLDER libraries)

    if(MSVC)
        set(SPIRV_IMPORTED_LOCATION
            IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/SPIRV/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRVd${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/SPIRV/Release/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/SPIRV/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/SPIRV/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}")
    else()
        set(SPIRV_IMPORTED_LOCATION
            IMPORTED_LOCATION "${BINARY_DIR}/SPIRV/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV.a")
    endif()

    if(MSVC)
        set(GLSLANG_IMPORTED_LOCATION
            IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/glslang/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}glslangd${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/glslang/Release/${CMAKE_FIND_LIBRARY_PREFIXES}glslang${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/glslang/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}glslang${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/glslang/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}glslang${CMAKE_FIND_LIBRARY_SUFFIXES}")
    else()
        set(GLSLANG_IMPORTED_LOCATION
            IMPORTED_LOCATION "${BINARY_DIR}/glslang/${CMAKE_FIND_LIBRARY_PREFIXES}glslang.a")
    endif()

    if(MSVC)
        set(GLSLANG_OSDEPENDENT_IMPORTED_LOCATION
            IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/glslang/OSDependent/Windows/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}OSDependentd${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/glslang/OSDependent/Windows/Release/${CMAKE_FIND_LIBRARY_PREFIXES}OSDependent${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/glslang/OSDependent/Windows/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}OSDependent${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/glslang/OSDependent/Windows/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}OSDependent${CMAKE_FIND_LIBRARY_SUFFIXES}")
    else()
        set(GLSLANG_OSDEPENDENT_IMPORTED_LOCATION
            IMPORTED_LOCATION "${BINARY_DIR}/glslang/OSDependent/Unix/${CMAKE_FIND_LIBRARY_PREFIXES}OSDependent.a")
    endif()

    if(MSVC)
        set(GLSLANG_OGLCOMPILER_IMPORTED_LOCATION
            IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/OGLCompilersDLL/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}OGLCompilerd${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/OGLCompilersDLL/Release/${CMAKE_FIND_LIBRARY_PREFIXES}OGLCompiler${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/OGLCompilersDLL/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}OGLCompiler${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/OGLCompilersDLL/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}OGLCompiler${CMAKE_FIND_LIBRARY_SUFFIXES}")
    else()
        set(GLSLANG_OGLCOMPILER_IMPORTED_LOCATION
            IMPORTED_LOCATION "${BINARY_DIR}/OGLCompilersDLL/${CMAKE_FIND_LIBRARY_PREFIXES}OGLCompiler.a")
    endif()

    add_library(spirv_import STATIC IMPORTED GLOBAL)
    add_dependencies(spirv_import glslang)
    set_target_properties(spirv_import PROPERTIES ${SPIRV_IMPORTED_LOCATION})

    add_library(spirv_final INTERFACE)
    add_dependencies(spirv_final spirv_import)
    target_include_directories(spirv_final INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/${GLSLANG_DIR}/SPIRV")
    target_link_libraries(spirv_final INTERFACE spirv_import)
    set(SPIRV_LIBRARIES spirv_final PARENT_SCOPE)

    add_library(glslang_import STATIC IMPORTED GLOBAL)
    add_dependencies(glslang_import glslang)
    set_target_properties(glslang_import PROPERTIES ${GLSLANG_IMPORTED_LOCATION})

    add_library(glslang_os_import STATIC IMPORTED GLOBAL)
    add_dependencies(glslang_os_import glslang)
    set_target_properties(glslang_os_import PROPERTIES ${GLSLANG_OSDEPENDENT_IMPORTED_LOCATION})

    add_library(glslang_oglcompiler_import STATIC IMPORTED GLOBAL)
    add_dependencies(glslang_oglcompiler_import glslang)
    set_target_properties(glslang_oglcompiler_import PROPERTIES ${GLSLANG_OGLCOMPILER_IMPORTED_LOCATION})

    add_library(glslang_final INTERFACE)
    add_dependencies(glslang_final glslang_import glslang_os_import)
    target_include_directories(glslang_final INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/${GLSLANG_DIR}/glslang/Public")
    target_include_directories(glslang_final INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/${GLSLANG_DIR}/glslang")
    target_link_libraries(glslang_final INTERFACE glslang_import glslang_os_import glslang_oglcompiler_import)
    set(GLSLANG_LIBRARIES glslang_final PARENT_SCOPE)
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
