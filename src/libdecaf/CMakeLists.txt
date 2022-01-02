project(libdecaf)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libdecaf STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources("Source Files" src)

target_compile_definitions(libdecaf PRIVATE
    DECAF_INSTALL_RESOURCESDIR="${DECAF_INSTALL_RESOURCESDIR}")

target_link_libraries(libdecaf
    addrlib
    common
    libcpu
    libgfd
    libgpu)

target_link_libraries(libdecaf
    cnl
    imgui::imgui
    c-ares::cares
    CURL::libcurl
    OpenSSL::SSL
    pugixml::pugixml
    unofficial::libuv::libuv
    ZLIB::ZLIB
    FFMPEG::AVCODEC
    FFMPEG::AVFILTER
    FFMPEG::AVUTIL
    FFMPEG::SWSCALE)

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
