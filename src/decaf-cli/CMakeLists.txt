project(decaf-cli)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(decaf-cli ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(decaf-cli
    common
    libconfig
    libdecaf
    excmd)

install(TARGETS decaf-cli RUNTIME DESTINATION "${DECAF_INSTALL_BINDIR}")

if(COMMAND x_vcpkg_install_local_dependencies)
   x_vcpkg_install_local_dependencies(TARGETS decaf-cli DESTINATION "${DECAF_INSTALL_BINDIR}")
endif()
