project(hardware-test)

include_directories(".")
include_directories("../../src/libdecaf/src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(hardware-test ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(hardware-test PROPERTIES FOLDER tools)

target_link_libraries(hardware-test
    common
    libdecaf)

install(TARGETS hardware-test RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/tests")
