include_directories(".")

set(SOURCE_FILES
    debugger_ui.cpp
    debugger_ui_draw.cpp
    )

set(HEADER_FILES
    debugger_ui.h
    )

add_library(debugger STATIC ${SOURCE_FILES} ${HEADER_FILES})
