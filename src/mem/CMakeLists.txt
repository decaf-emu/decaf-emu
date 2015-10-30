include_directories(".")

set(SOURCE_FILES
	mem.cpp
    )
set(HEADER_FILES
	mem.h
    )

add_library(mem STATIC ${SOURCE_FILES} ${HEADER_FILES})
