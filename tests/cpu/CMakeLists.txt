project(tests-cpu)

add_subdirectory("libcpu")
add_subdirectory("runner-achurch")
add_subdirectory("runner-generated")

install(DIRECTORY "data/"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/cpu/data")
