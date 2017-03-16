project(hardware-test-generator)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(hardware-test-generator ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(hardware-test-generator PROPERTIES FOLDER tools)

target_link_libraries(hardware-test-generator
    common
    libcpu)

install(TARGETS hardware-test-generator RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/tests")
