project(tools)
include_directories(".")
include_directories("../src")

add_subdirectory(gfd-tool)

if(DECAF_SDL)
    add_subdirectory(pm4-replay)
endif()

if(DECAF_QT)
    add_subdirectory(pm4-replay-qt)
endif()
