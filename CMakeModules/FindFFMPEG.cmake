macro(find_component COMPONENT LIBRARY HEADER)
   find_path(${COMPONENT}_INCLUDE_DIR NAMES "${HEADER}" HINTS "/usr/include/ffmpeg")
   find_library(${COMPONENT}_LIBRARY NAMES "${LIBRARY}")

   if(${COMPONENT}_INCLUDE_DIR AND ${COMPONENT}_LIBRARY)
      set(${COMPONENT}_FOUND TRUE)

      if(NOT TARGET FFMPEG::${COMPONENT})
        add_library(FFMPEG::${COMPONENT} UNKNOWN IMPORTED)
        set_target_properties(FFMPEG::${COMPONENT} PROPERTIES
           IMPORTED_LOCATION "${${COMPONENT}_LIBRARY}"
           INTERFACE_INCLUDE_DIRECTORIES "${${COMPONENT}_INCLUDE_DIR}")
      endif()
   else()
      set(${COMPONENT}_FOUND FALSE)
   endif()
endmacro()

find_component(AVCODEC "avcodec" "libavcodec/avcodec.h")
find_component(AVFILTER "avfilter" "libavfilter/avfilter.h")
find_component(AVUTIL "avutil" "libavutil/avutil.h")
find_component(SWSCALE "swscale" "libswscale/swscale.h")
