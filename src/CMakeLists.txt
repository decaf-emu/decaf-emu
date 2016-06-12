include_directories(".")

set(SOURCE_FILES
    debugcontrol.cpp
    debugger.cpp
    debugnet.cpp
    emulog.cpp
    )
set(HEADER_FILES
    debugcontrol.h
    debugger.h
    debugmsg.h
    debugnet.h
    traceiter.h
    types.h
    usermodule.h
    )

add_subdirectory(common)
add_subdirectory(cpu)
add_subdirectory(debugger)
add_subdirectory(decaf)
add_subdirectory(decaf-cli)
add_subdirectory(filesystem)
add_subdirectory(gpu)
add_subdirectory(input)
add_subdirectory(kernel)
add_subdirectory(modules)
add_subdirectory(platform)
add_subdirectory(ppcutils)

add_executable(decaf-emu ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(decaf-emu common cpu decaf-cli decaf-api filesystem input modules kernel modules kernel gpu debugger platform ppcutils)
target_link_libraries(decaf-emu z m ${ADDRLIB_LIBRARIES} ${ASMJIT_LIBRARIES} ${GLBINDING_LIBRARIES} ${IMGUI_LIBRARIES} ${GLFW_LIBRARIES} ${PUGIXML_LIBRARIES} ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_LIBRARIES})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(decaf-emu X11)
endif ()
install(TARGETS decaf-emu RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
