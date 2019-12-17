project(libcpu)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

if(MSVC)
    add_library(libcpu STATIC ${SOURCE_FILES} ${HEADER_FILES} "cpu.natvis")
else()
    add_library(libcpu STATIC ${SOURCE_FILES} ${HEADER_FILES})
endif()

GroupSources("Header Files/expresso" espresso)
GroupSources("Source Files" src)

target_link_libraries(libcpu
    common
    binrec)

if(MSVC)
    target_link_libraries(libcpu
        ntdll)
else()
    target_link_libraries(libcpu
        m
        ${CMAKE_THREAD_LIBS_INIT})
endif()

if(DECAF_PCH)
    target_precompile_headers(libcpu
      PRIVATE
        <common/pch.h>
    )

    AutoGroupPCHFiles()
endif()
