include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(test-libcpu ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(test-libcpu PROPERTIES FOLDER tests)

target_link_libraries(test-libcpu
    catch2
    common
    libcpu)

add_test(NAME tests_libcpu
         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
         COMMAND test-libcpu)
