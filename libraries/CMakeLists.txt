project(libraries)
include(ExternalProject)

find_package(PkgConfig QUIET)

# addrlib
set(ADDRLIB_DIR "addrlib")
add_library(addrlib STATIC
    "${ADDRLIB_DIR}/src/addrinterface.cpp"
    "${ADDRLIB_DIR}/src/core/addrelemlib.cpp"
    "${ADDRLIB_DIR}/src/core/addrlib.cpp"
    "${ADDRLIB_DIR}/src/core/addrobject.cpp"
    "${ADDRLIB_DIR}/src/r600/r600addrlib.cpp")
set_target_properties(addrlib PROPERTIES FOLDER libraries)
target_include_directories(addrlib
   PRIVATE "${ADDRLIB_DIR}/src"
   PUBLIC "${ADDRLIB_DIR}/include")
set(ADDRLIB_LIBRARIES addrlib PARENT_SCOPE)

# libbinrec
set(BINREC_ENABLE_RTL_DEBUG_OPTIMIZE FALSE CACHE BOOL "Enable debug output from optimization passes")
set(BINREC_ENABLE_ASSERT FALSE CACHE BOOL "Enable basic assertion checks")
add_subdirectory(libbinrec)
set_target_properties(binrec PROPERTIES FOLDER libraries)
set(BINREC_LIBRARIES binrec PARENT_SCOPE)

# catch
set(CATCH_DIR "catch")
add_library(catch INTERFACE)
target_include_directories(catch INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${CATCH_DIR}/include")
set(CATCH_LIBRARIES catch PARENT_SCOPE)

# cereal
set(CEREAL_DIR "cereal")
add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${CEREAL_DIR}/include")
set(CEREAL_LIBRARIES cereal PARENT_SCOPE)

# cpptoml
set(CPPTOML_DIR "cpptoml")
add_library(cpptoml INTERFACE)
target_include_directories(cpptoml INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${CPPTOML_DIR}/include")
target_compile_definitions(cpptoml INTERFACE "CPPTOML_USE_MAP")
set(CPPTOML_LIBRARIES cpptoml PARENT_SCOPE)

# excmd
set(EXCMD_DIR "excmd")
add_library(excmd INTERFACE)
target_include_directories(excmd INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${EXCMD_DIR}/src")
set(EXCMD_LIBRARIES excmd PARENT_SCOPE)

# fixed_point
set(FIXED_POINT_DIR "fixed_point")
add_library(fixed_point INTERFACE)
target_include_directories(fixed_point INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${FIXED_POINT_DIR}/include")
set(FIXED_POINT_LIBRARIES fixed_point PARENT_SCOPE)

# fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES FOLDER libraries)
set(FMT_LIBRARIES fmt PARENT_SCOPE)

# glbinding
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

# gsl
set(GSL_DIR "gsl-lite")
add_library(gsl INTERFACE)
target_include_directories(gsl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${GSL_DIR}/include")
target_compile_definitions(gsl INTERFACE "GSL_THROWS_FOR_TESTING")
set(GSL_LIBRARIES gsl PARENT_SCOPE)

# imgui
set(IMGUI_DIR "imgui")
include_directories("${IMGUI_DIR}")
add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp")
set_target_properties(imgui PROPERTIES FOLDER libraries)
target_include_directories(imgui INTERFACE "${IMGUI_DIR}")
set(IMGUI_LIBRARIES imgui PARENT_SCOPE)

# ovsocket
set(OVSOCKET_DIR "ovsocket")
add_library(ovsocket INTERFACE)
target_include_directories(ovsocket INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${OVSOCKET_DIR}/ovsocket")
set(OVSOCKET_LIBRARIES ovsocket PARENT_SCOPE)

# peglib
set(PEGLIB_DIR "cpp-peglib")
add_library(peglib INTERFACE)
target_include_directories(peglib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${PEGLIB_DIR}")
set(PEGLIB_LIBRARIES peglib PARENT_SCOPE)

# pugixml
set(PUGIXML_DIR "pugixml")
add_library(pugixml STATIC
   "${PUGIXML_DIR}/src/pugixml.cpp")
set_target_properties(pugixml PROPERTIES FOLDER libraries)
target_include_directories(pugixml INTERFACE "${PUGIXML_DIR}/src")
set(PUGIXML_LIBRARIES pugixml PARENT_SCOPE)

# sdl2
if(DECAF_SDL)
    if(PKGCONFIG_FOUND)
        pkg_search_module(SDL2 QUIET sdl2)
    endif()

    if(NOT SDL2_FOUND)
        set(SDL2_DIR "SDL-mirror")
        include_directories("${SDL2_DIR}/include")
        externalproject_add(sdl2
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_DIR}"
            CMAKE_CACHE_ARGS
                -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
                -DCMAKE_C_FLAGS:string=${CMAKE_C_FLAGS}
                -DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}
                -DCMAKE_CXX_FLAGS:string=${CMAKE_CXX_FLAGS}
                -DSDL_STATIC:string=off
                -DSDL_SHARED:string=on
            INSTALL_COMMAND "")
        externalproject_get_property(sdl2 BINARY_DIR)
        externalproject_add_step(sdl2
            copy_to_binary
            DEPENDEES build
            DEPENDERS install
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
        set_target_properties(sdl2 PROPERTIES FOLDER libraries)

        if(MSVC)
            set(SDL2_IMPORTED_LOCATION
                IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}sdl2${CMAKE_FIND_LIBRARY_SUFFIXES}"
                IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/Release/${CMAKE_FIND_LIBRARY_PREFIXES}sdl2${CMAKE_FIND_LIBRARY_SUFFIXES}"
                IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}sdl2${CMAKE_FIND_LIBRARY_SUFFIXES}"
                IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}sdl2${CMAKE_FIND_LIBRARY_SUFFIXES}")
        else()
            set(SDL2_IMPORTED_LOCATION
                IMPORTED_LOCATION "${BINARY_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}sdl2.a")
        endif()

        add_library(sdl2_import STATIC IMPORTED GLOBAL)
        add_dependencies(sdl2_import sdl2)
        set_target_properties(sdl2_import PROPERTIES ${SDL2_IMPORTED_LOCATION})

        set(SDL2_LIBRARIES sdl2_import)
        set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${SDL2_DIR}/include")

        install(FILES "${BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${CMAKE_FIND_LIBRARY_PREFIXES}sdl2${CMAKE_SHARED_LIBRARY_SUFFIX}"
            DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif()

    add_library(sdl2_final INTERFACE)
    target_compile_definitions(sdl2_final INTERFACE "SDL_MAIN_HANDLED" "HAVE_LIBC")
    target_include_directories(sdl2_final INTERFACE ${SDL2_INCLUDE_DIRS})
    if(SDL2_FOUND)
        target_link_libraries(sdl2_final INTERFACE ${SDL2_LDFLAGS} ${SDL2_LDFLAGS_OTHER})
    else()
        target_link_libraries(sdl2_final INTERFACE ${SDL2_LIBRARIES})
    endif()
    set(SDL2_LINK sdl2_final PARENT_SCOPE)

    if(MSVC)
        target_link_libraries(sdl2_final INTERFACE version Winmm)
    endif()
else()
    set(SDL2_LINK "" PARENT_SCOPE)
endif()


# spdlog
set(SPDLOG_DIR "spdlog")
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/${SPDLOG_DIR}/include")
target_compile_definitions(spdlog INTERFACE "SPDLOG_FMT_EXTERNAL")
target_link_libraries(spdlog INTERFACE ${FMT_LIBRARIES})
set(SPDLOG_LIBRARIES spdlog PARENT_SCOPE)

# vulkan
if(Vulkan_FOUND)
    add_library(vulkan INTERFACE)
    target_link_libraries(vulkan INTERFACE ${Vulkan_LIBRARIES})
    target_include_directories(vulkan INTERFACE ${Vulkan_INCLUDE_DIRS})
    if(MSVC)
        target_compile_definitions(vulkan INTERFACE "VK_USE_PLATFORM_WIN32_KHR")
    endif()
    set(VULKAN_LIBRARIES vulkan PARENT_SCOPE)
else()
    set(VULKAN_LIBRARIES "" PARENT_SCOPE)
endif()

# zlib
find_package(ZLIB QUIET)

if(NOT ZLIB_FOUND)
    set(ZLIB_DIR "zlib")
    externalproject_add(zlib
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ZLIB_DIR}"
        PATCH_COMMAND ${CMAKE_COMMAND} -E remove <SOURCE_DIR>/zconf.h
        CMAKE_CACHE_ARGS
            -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
            -DCMAKE_C_FLAGS:string=${CMAKE_C_FLAGS}
            -DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}
            -DCMAKE_CXX_FLAGS:string=${CMAKE_CXX_FLAGS}
            -DBUILD_SHARED_LIBS:string=on
        INSTALL_COMMAND ""
        )
    externalproject_get_property(zlib BINARY_DIR)
    externalproject_add_step(zlib
        copy_to_binary
        DEPENDEES build
        DEPENDERS install
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${BINARY_DIR}/${CMAKE_CFG_INTDIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}")
    set_target_properties(zlib PROPERTIES FOLDER libraries)

    if(MSVC)
        set(ZLIB_IMPORTED_LOCATION
            IMPORTED_LOCATION_DEBUG "${BINARY_DIR}/Debug/${CMAKE_FIND_LIBRARY_PREFIXES}zlibd${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELEASE "${BINARY_DIR}/Release/${CMAKE_FIND_LIBRARY_PREFIXES}zlib${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${BINARY_DIR}/RelWithDebInfo/${CMAKE_FIND_LIBRARY_PREFIXES}zlib${CMAKE_FIND_LIBRARY_SUFFIXES}"
            IMPORTED_LOCATION_MINSIZEREL "${BINARY_DIR}/MinSizeRel/${CMAKE_FIND_LIBRARY_PREFIXES}zlib${CMAKE_FIND_LIBRARY_SUFFIXES}")
    else()
        set(ZLIB_IMPORTED_LOCATION
            IMPORTED_LOCATION "${BINARY_DIR}/${CMAKE_FIND_LIBRARY_PREFIXES}z.a")
    endif()

    add_library(zlib_import STATIC IMPORTED GLOBAL)
    add_dependencies(zlib_import zlib)
    set_target_properties(zlib_import PROPERTIES ${ZLIB_IMPORTED_LOCATION})

    set(ZLIB_LIBRARIES zlib_import)
    set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/${ZLIB_DIR} ${BINARY_DIR})

    install(FILES "${BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${CMAKE_FIND_LIBRARY_PREFIXES}zlib$<$<CONFIG:Debug>:d>1${CMAKE_SHARED_LIBRARY_SUFFIX}"
        DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()

add_library(zlib_final INTERFACE)
target_include_directories(zlib_final INTERFACE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(zlib_final INTERFACE ${ZLIB_LIBRARIES})
set(ZLIB_LINK zlib_final PARENT_SCOPE)
