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

# cpptoml
add_library(cpptoml INTERFACE IMPORTED GLOBAL)
set_target_properties(cpptoml PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "CPPTOML_USE_MAP"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cpptoml/include")

# excmd
add_library(excmd INTERFACE IMPORTED GLOBAL)
set_target_properties(excmd PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/excmd/src")

# fixed_point
add_library(fixed_point INTERFACE IMPORTED GLOBAL)
set_target_properties(fixed_point PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/fixed_point/include")

# fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER libraries)

# glbinding
if(DECAF_GL)
    find_package(glbinding QUIET)

    if(NOT glbinding_FOUND)
        set(GLBINDING_DIR "glbinding")
        set(GLBINDING_CXX_FLAGS ${CMAKE_CXX_FLAGS})

        if (MSVC)
            set(GLBINDING_BUILD_SHARED on)
            set(GLBINDING_CXX_FLAGS "${GLBINDING_CXX_FLAGS} -D_SILENCE_CXX17_ADAPTOR_TYPEDEFS_DEPRECATION_WARNING")
        else ()
            set(GLBINDING_BUILD_SHARED off)
        endif ()

        externalproject_add(glbinding
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${GLBINDING_DIR}"
            CMAKE_CACHE_ARGS
                -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
                -DCMAKE_C_FLAGS:string=${CMAKE_C_FLAGS}
                -DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}
                -DCMAKE_CXX_FLAGS:string=${GLBINDING_CXX_FLAGS}
                -DBUILD_SHARED_LIBS:string=${GLBINDING_BUILD_SHARED}
                -DOPTION_BUILD_DOCS:string=off
                -DOPTION_BUILD_EXAMPLES:string=off
                -DOPTION_BUILD_TESTS:string=off
                -DOPTION_BUILD_TOOLS:string=off
                -DOPTION_BUILD_GPU_TESTS:string=off
            INSTALL_COMMAND "")
        externalproject_get_property(glbinding BINARY_DIR)
        externalproject_add_step(glbinding
            copy_to_binary
            DEPENDEES build
            DEPENDERS install
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
        set_target_properties(glbinding PROPERTIES FOLDER libraries)

        if (MSVC)
            set(GLBINDING_IMPORTED_LOCATION
                IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}glbindingd${CMAKE_FIND_LIBRARY_SUFFIXES}"
                IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/Release/${CMAKE_FIND_LIBRARY_PREFIXES}glbinding${CMAKE_FIND_LIBRARY_SUFFIXES}"
                IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}glbinding${CMAKE_FIND_LIBRARY_SUFFIXES}"
                IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}glbinding${CMAKE_FIND_LIBRARY_SUFFIXES}")
        else ()
            set(GLBINDING_IMPORTED_LOCATION
                IMPORTED_LOCATION "${BINARY_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}glbinding.a")
        endif ()

        add_library(glbinding_import STATIC IMPORTED GLOBAL)
        add_dependencies(glbinding_import glbinding)
        set_target_properties(glbinding_import PROPERTIES ${GLBINDING_IMPORTED_LOCATION})

        add_library(glbinding_final INTERFACE)
        add_dependencies(glbinding_final glbinding_import)
        target_include_directories(glbinding_final INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/${GLBINDING_DIR}/source/glbinding/include"
            "${BINARY_DIR}/source/glbinding/include")
        target_link_libraries(glbinding_final INTERFACE glbinding_import)
        set(GLBINDING_LIBRARIES glbinding_final PARENT_SCOPE)

        if (MSVC)
            install(FILES "${BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${CMAKE_FIND_LIBRARY_PREFIXES}glbinding$<$<CONFIG:Debug>:d>${CMAKE_SHARED_LIBRARY_SUFFIX}"
                    DESTINATION "${CMAKE_INSTALL_PREFIX}")
        endif ()
    else()
        set(GLBINDING_LIBRARIES glbinding PARENT_SCOPE)
    endif()
endif()

# glslang
if(DECAF_VULKAN)
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
        copy_to_binary
        DEPENDEES build
        DEPENDERS install
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/SPIRV/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
    set_target_properties(glslang PROPERTIES FOLDER libraries)

    if (MSVC)
        set(SPIRV_IMPORTED_LOCATION
            IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/SPIRV/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/SPIRV/Release/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/SPIRV/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/SPIRV/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV${CMAKE_FIND_LIBRARY_SUFFIXES}")
    else ()
        set(SPIRV_IMPORTED_LOCATION
            IMPORTED_LOCATION "${BINARY_DIR}/SPIRV/${CMAKE_FIND_LIBRARY_PREFIXES}SPIRV.a")
    endif ()

    add_library(spirv_import STATIC IMPORTED GLOBAL)
    add_dependencies(spirv_import glslang)
    set_target_properties(spirv_import PROPERTIES ${SPIRV_IMPORTED_LOCATION})

    add_library(spirv_final INTERFACE)
    add_dependencies(spirv_final spirv_import)
    target_include_directories(spirv_final INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/${GLSLANG_DIR}/SPIRV")
    target_link_libraries(spirv_final INTERFACE spirv_import)
    set(SPIRV_LIBRARIES spirv_final PARENT_SCOPE)
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
add_library(spdlog INTERFACE IMPORTED GLOBAL)
set_target_properties(spdlog PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS "SPDLOG_FMT_EXTERNAL"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include"
    INTERFACE_LINK_LIBRARIES fmt)
