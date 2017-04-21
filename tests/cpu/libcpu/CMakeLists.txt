include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(test-libcpu ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(test-libcpu PROPERTIES FOLDER tools)

target_link_libraries(test-libcpu
    catch
    common
    libcpu)

install(TARGETS test-libcpu RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/cpu")

add_test(NAME tests_libcpu
         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
         COMMAND test-libcpu)
