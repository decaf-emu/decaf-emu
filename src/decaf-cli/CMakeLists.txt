include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(decaf-cli ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(decaf-cli
    libdecaf
    libcpu
    common)

target_link_libraries(decaf-cli
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
    target_link_libraries(decaf-cli X11)
endif ()

install(TARGETS decaf-cli RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
