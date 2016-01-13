include_directories(".")

set(SOURCE_FILES
    crc32.cpp
    log.cpp
    wfunc_ptr.cpp
    )
set(HEADER_FILES
    align.h
    be_array.h
    be_val.h
    be_vec.h
    be_volatile.h
    bigendianview.h
    binaryfile.h
    binaryview.h
    bit_cast.h
    bitutils.h
    byte_swap.h
    crc32.h
    debuglog.h
    fixed.h
    floatutils.h
    log.h
    make_array.h
    structsize.h
    strutils.h
    teenyheap.h
    type_list.h
    virtual_ptr.h
    wfunc_call.h
    wfunc_ptr.h
    )

add_library(utils STATIC ${SOURCE_FILES} ${HEADER_FILES})
