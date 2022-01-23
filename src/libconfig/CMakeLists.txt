project(libconfig)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libconfig STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources("Source Files" src)

target_link_libraries(libconfig
    common
    libcpu
    libdecaf
    libgpu
    tomlplusplus
    excmd)
