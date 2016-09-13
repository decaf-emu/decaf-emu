# Can't use -Werror=shadow because of excmd
set(CMAKE_CXX_FLAGS "-Wshadow ${CMAKE_CXX_FLAGS}")

include_directories(".")
include_directories("../../src/libdecaf/src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(pm4-replay ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(pm4-replay
    libdecaf
    libcpu
    common)

target_link_libraries(pm4-replay
    z
    m
    ${ADDRLIB_LIBRARIES}
    ${ASMJIT_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLBINDING_LIBRARIES}
    ${IMGUI_LIBRARIES}
    ${PUGIXML_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(pm4-replay X11)
endif ()

install(TARGETS pm4-replay RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
