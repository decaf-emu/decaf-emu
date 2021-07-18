project(libdecaf)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libdecaf STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources("Source Files" src)

target_compile_definitions(libdecaf PRIVATE
    DECAF_INSTALL_RESOURCESDIR="${DECAF_INSTALL_RESOURCESDIR}")

target_include_directories(libdecaf PRIVATE
    ${CURL_INCLUDE_DIRS})

target_link_libraries(libdecaf
    addrlib
    common
    cnl
    imgui
    libcpu
    libgfd
    libgpu
    CARES::CARES
    LibUV::LibUV
    OpenSSL::SSL
    pugixml
    ZLIB::ZLIB
    ${CURL_LIBRARIES})

if(DECAF_FFMPEG)
    target_link_libraries(libdecaf
        FFMPEG::AVCODEC
        FFMPEG::AVFILTER
        FFMPEG::AVUTIL
        FFMPEG::SWSCALE)
endif()

if(MSVC)
    target_link_libraries(libdecaf Crypt32 ws2_32 Psapi IPHLPAPI userenv)
    target_compile_options(libdecaf PUBLIC /wd4251)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(libdecaf dl)
endif()

if(DECAF_PCH)
    target_precompile_headers(libdecaf
      PRIVATE
        <common/pch.h>
    )

    AutoGroupPCHFiles()
endif()
