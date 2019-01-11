project(gfd-tool)

include_directories(".")
include_directories("../../src/libdecaf/src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(gfd-tool ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(gfd-tool PROPERTIES FOLDER tools)

target_link_libraries(gfd-tool
    common
    libdecaf
    libgfd
    excmd)

install(TARGETS gfd-tool RUNTIME DESTINATION "${DECAF_INSTALL_BINDIR}")
