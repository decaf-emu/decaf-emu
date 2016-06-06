include_directories(".")

set(SOURCE_FILES
    kernel.cpp
    kernel_fibers.cpp
    kernel_hle_modules.cpp
    )

set(HEADER_FILES
    kernel.h
    )

add_library(kernel STATIC ${SOURCE_FILES} ${HEADER_FILES})
