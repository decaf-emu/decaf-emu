project(decaf-cli)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(decaf-cli ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(decaf-cli
    common
    libconfig
    libdecaf
    cpptoml
    excmd)

install(TARGETS decaf-cli RUNTIME DESTINATION "${DECAF_INSTALL_BINDIR}")
