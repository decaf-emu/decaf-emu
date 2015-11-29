include_directories(".")

set(SOURCE_FILES
    input.cpp
    )

set(HEADER_FILES
    input.h
    )

add_library(input STATIC ${SOURCE_FILES} ${HEADER_FILES})
