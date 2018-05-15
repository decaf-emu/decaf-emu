project(pm4-replay-qt)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(".")
include_directories("../../src/libdecaf/src")
include_directories("../../src/libgpu")
include_directories("../../src/libgpu/src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)
file(GLOB_RECURSE UI_FILES *.ui)

qt5_wrap_ui(UIS_HDRS ${UI_FILES})

add_executable(pm4-replay-qt ${SOURCE_FILES} ${HEADER_FILES} ${UIS_HDRS})
set_target_properties(pm4-replay-qt PROPERTIES FOLDER tools)

target_link_libraries(pm4-replay-qt
    common
    libdecaf
    ${EXCMD_LIBRARIES})

target_link_libraries(pm4-replay-qt Qt5::Widgets)

install(TARGETS pm4-replay-qt RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
