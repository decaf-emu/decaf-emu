include_directories(".")

set(SOURCE_FILES
    )
set(HEADER_FILES
    stackobject.h
    wfunc_call.h
    wfunc_ptr.h
    )

add_library(ppcutils STATIC ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(ppcutils PROPERTIES LINKER_LANGUAGE CXX)
