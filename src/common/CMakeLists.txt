project(common)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h *.inl)

add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources("Source Files" src)

target_link_libraries(common
    cnl
    fmt::fmt
    gsl
    spdlog)

if(MSVC)
    target_link_libraries(common Dbghelp)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(common rt)
endif()

if(DECAF_PCH)
    target_precompile_headers(common
      PRIVATE
        "pch.h"
    )

    AutoGroupPCHFiles()
endif()
