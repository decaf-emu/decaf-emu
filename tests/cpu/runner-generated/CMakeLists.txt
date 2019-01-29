include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(runner-generated ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(runner-generated PROPERTIES FOLDER tests)

target_link_libraries(runner-generated
    common
    libcpu
    libdecaf
    cereal)

add_test(NAME tests_cpu_generated
         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
         COMMAND runner-generated)
