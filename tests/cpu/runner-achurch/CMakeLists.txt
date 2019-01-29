include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(runner-achurch ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(runner-achurch PROPERTIES FOLDER tests)

target_link_libraries(runner-achurch
    common
    libcpu)

add_test(NAME tests_cpu_achurch
         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
         COMMAND runner-achurch)
