include_directories(".")

set(SOURCE_FILES
    config.cpp
    debugcontrol.cpp
    debugger.cpp
    debugnet.cpp
    elf.cpp
    fuzztests.cpp
    hardwaretests.cpp
    loader.cpp
    main.cpp
    memory_translate.cpp
    processor.cpp
    system.cpp
    )
set(HEADER_FILES
    config.h
    debugcontrol.h
    debugger.h
    debugmsg.h
    debugnet.h
    elf.h
    fuzztests.h
    hardwaretests.h
    hostlookup.h
    kerneldata.h
    kernelexport.h
    kernelfunction.h
    kernelmodule.h
    loader.h
    memory_translate.h
    ppcinvokeargs.h
    ppcinvoke.h
    ppcinvokelog.h
    ppcinvokeresult.h
    ppctypes.h
    processor.h
    system.h
    traceiter.h
    types.h
    usermodule.h
    )

add_subdirectory(cpu)
add_subdirectory(filesystem)
add_subdirectory(gpu)
add_subdirectory(input)
add_subdirectory(mem)
add_subdirectory(modules)
add_subdirectory(platform)
add_subdirectory(utils)

add_executable(decaf-emu ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(decaf-emu cpu filesystem gpu input mem modules platform utils)
target_link_libraries(decaf-emu z ${ASMJIT_LIBRARIES} ${DOCOPT_LIBRARIES} ${PUGIXML_LIBRARIES} ${GLBINDING_LIBRARIES} ${GLFW_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
