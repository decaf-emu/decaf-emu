project(hardware-test-achurch)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(hardware-test-achurch ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(hardware-test-achurch PROPERTIES FOLDER tools)

target_link_libraries(hardware-test-achurch
    common
    libcpu)

install(TARGETS hardware-test-achurch RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
