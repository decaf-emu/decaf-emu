project(libcpu)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libcpu STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources(espresso)
GroupSources(src)

target_link_libraries(libcpu
    common
    ${BINREC_LIBRARIES})

if(MSVC)
    target_link_libraries(libcpu
        ntdll)
else()
    target_link_libraries(libcpu
        m
        ${CMAKE_THREAD_LIBS_INIT})
endif()
