include_directories(".")

set(SOURCE_FILES
    decaf.cpp
    )

set(HEADER_FILES
    decaf.h
    decaf_input.h
    )

add_library(decaf-api STATIC ${SOURCE_FILES} ${HEADER_FILES})
