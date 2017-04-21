include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(runner-achurch ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(runner-achurch PROPERTIES FOLDER tools)

target_link_libraries(runner-achurch
    common
    libcpu)

install(TARGETS runner-achurch RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/cpu")
