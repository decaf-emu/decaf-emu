include_directories(".")

set(SOURCE_FILES
    kernel.cpp
    kernel_fibers.cpp
    kernel_filesystem.cpp
    kernel_hle.cpp
    )

set(HEADER_FILES
    kernel.h
    kernel_filesystem.h
    kernel_internal.h
    kernel_hle.h
    kernel_hledata.h
    kernel_hlesymbol.h
    kernel_hlefunction.h
    kernel_hlemodule.h
    )

add_library(kernel STATIC ${SOURCE_FILES} ${HEADER_FILES})
