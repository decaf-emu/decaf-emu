project(libgpu)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libgpu STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources(latte)
GroupSources(src)

target_link_libraries(libgpu
    common
    libcpu
    ${ADDRLIB_LIBRARIES})

if(APPLE)
    target_compile_definitions(libgpu PUBLIC DECAF_NOGL)
else()
    target_link_libraries(libgpu
        ${GLBINDING_LIBRARIES}
        ${OPENGL_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(libgpu PUBLIC /wd4251)
endif()
