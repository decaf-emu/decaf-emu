project(tools)
include_directories(".")
include_directories("../src")

add_subdirectory(gfd-tool)
add_subdirectory(hardware-test)
add_subdirectory(hardware-test-generator)
add_subdirectory(hwtest-achurch)
add_subdirectory(pm4-replay)
