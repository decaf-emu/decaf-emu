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

# cnl
add_library(cnl INTERFACE IMPORTED GLOBAL)
set_target_properties(cnl PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cnl/include")

# excmd
add_library(excmd INTERFACE IMPORTED GLOBAL)
set_target_properties(excmd PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/excmd/src")

# ovsocket
add_library(ovsocket INTERFACE IMPORTED GLOBAL)
set_target_properties(ovsocket PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/ovsocket/ovsocket")

# peglib
add_library(peglib INTERFACE IMPORTED GLOBAL)
set_target_properties(peglib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/cpp-peglib")

# Qt Advanced Docking System
if(DECAF_QT)
    set(BUILD_STATIC TRUE CACHE BOOL "ADS: Build the static library")
    set(BUILD_EXAMPLES FALSE CACHE BOOL "ADS: Build the examples")
    set(ADS_VERSION "3.8.1" CACHE STRING "ADS: Version")
    add_subdirectory(qtads EXCLUDE_FROM_ALL)
    set_target_properties(qtadvanceddocking PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "ADS_STATIC"
        FOLDER libraries)
endif()
