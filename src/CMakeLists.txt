include_directories(".")

set(SOURCE_FILES
    config.cpp
    debugcontrol.cpp
    debugger.cpp
    debugnet.cpp
    elf.cpp
    emulog.cpp
    loader.cpp
    main.cpp
    system.cpp
    )
set(HEADER_FILES
    config.h
    debugcontrol.h
    debugger.h
    debugmsg.h
    debugnet.h
    elf.h
    hostlookup.h
    kerneldata.h
    kernelexport.h
    kernelfunction.h
    kernelmodule.h
    loader.h
    ppcinvokeargs.h
    ppcinvoke.h
    ppcinvokelog.h
    ppcinvokeresult.h
    ppctypes.h
    system.h
    traceiter.h
    types.h
    usermodule.h
    )

add_subdirectory(common)
add_subdirectory(cpu)
add_subdirectory(filesystem)
add_subdirectory(gpu)
add_subdirectory(input)
add_subdirectory(kernel)
add_subdirectory(modules)
add_subdirectory(platform)
add_subdirectory(ppcutils)

add_executable(decaf-emu ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(decaf-emu common cpu filesystem input kernel modules gpu platform ppcutils)
target_link_libraries(decaf-emu z m ${ADDRLIB_LIBRARIES} ${ASMJIT_LIBRARIES} ${DOCOPT_LIBRARIES} ${GLBINDING_LIBRARIES} ${GLFW_LIBRARIES} ${PUGIXML_LIBRARIES} ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_LIBRARIES})
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(decaf-emu X11)
endif ()
install(TARGETS decaf-emu RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
