include_directories(".")

set(SOURCE_FILES
    config.cpp
    gl_common.cpp
    main.cpp
    main_glfw.cpp
    main_sdl.cpp
    )

set(HEADER_FILES
    clilog.h
    config.h
    gl_common.h
    input_common.h
    )

add_library(decaf-cli STATIC ${SOURCE_FILES} ${HEADER_FILES})
