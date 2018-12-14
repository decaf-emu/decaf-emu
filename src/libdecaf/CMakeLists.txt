project(libdecaf)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libdecaf STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources(src)

target_link_libraries(libdecaf
    common
    libcpu
    libgfd
    libgpu
    addrlib
    fixed_point
    imgui
    pugixml
    ZLIB::ZLIB)

if(DECAF_FFMPEG)
    target_link_libraries(libdecaf
        FFMPEG::AVCODEC
        FFMPEG::AVFILTER
        FFMPEG::AVUTIL
        FFMPEG::SWSCALE)
endif()

if(MSVC)
    target_link_libraries(libdecaf ws2_32)
    target_compile_options(libdecaf PUBLIC /wd4251)
endif()
