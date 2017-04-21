project(tools)
include_directories(".")
include_directories("../src")

add_subdirectory(gfd-tool)
if(NOT APPLE)
    add_subdirectory(pm4-replay)
endif()
