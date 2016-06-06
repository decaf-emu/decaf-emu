include_directories(".")

set(SOURCE_FILES
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
    emuassert.h
    fixed.h
    floatutils.h
    log.h
    make_array.h
    structsize.h
    strutils.h
    teenyheap.h
    type_list.h
    virtual_ptr.h
    )

add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES})
