project(common)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources(src)

target_link_libraries(common
    fixed_point
    fmt::fmt
    gsl
    spdlog)

if(MSVC)
    target_link_libraries(common Dbghelp)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(common rt)
endif()
