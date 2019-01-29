project(libdecaf)

include_directories(".")
include_directories("src")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(libdecaf STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources(src)

target_compile_definitions(libdecaf PRIVATE
    DECAF_INSTALL_RESOURCESDIR="${DECAF_INSTALL_RESOURCESDIR}")

target_include_directories(libdecaf PRIVATE
    ${CURL_INCLUDE_DIRS})

target_link_libraries(libdecaf
    addrlib
    common
    fixed_point
    imgui
    libcpu
    libgfd
    libgpu
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

    if(MSVC AND USING_CONAN)
        add_custom_command(TARGET libdecaf POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CONAN_BIN_DIRS_FFMPEG}/avcodec-58.dll"
                $<TARGET_FILE_DIR:libdecaf>)
        add_custom_command(TARGET libdecaf POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CONAN_BIN_DIRS_FFMPEG}/avutil-56.dll"
                $<TARGET_FILE_DIR:libdecaf>)
        add_custom_command(TARGET libdecaf POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CONAN_BIN_DIRS_FFMPEG}/swresample-3.dll"
                $<TARGET_FILE_DIR:libdecaf>)
        add_custom_command(TARGET libdecaf POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CONAN_BIN_DIRS_FFMPEG}/swscale-5.dll"
                $<TARGET_FILE_DIR:libdecaf>)

       install(FILES
          "${CONAN_BIN_DIRS_FFMPEG}/avcodec-58.dll"
          "${CONAN_BIN_DIRS_FFMPEG}/avutil-56.dll"
          "${CONAN_BIN_DIRS_FFMPEG}/swresample-3.dll"
          "${CONAN_BIN_DIRS_FFMPEG}/swscale-5.dll"
          DESTINATION "${DECAF_INSTALL_BINDIR}")
    endif()
endif()

if(MSVC)
    target_link_libraries(libdecaf ws2_32)
    target_compile_options(libdecaf PUBLIC /wd4251)
endif()

enable_pch(libdecaf)
