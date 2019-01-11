project(resources)

set(Fonts
   fonts/CafeCn.ttf
   fonts/CafeKr.ttf
   fonts/CafeStd.ttf
   fonts/CafeTw.ttf
   fonts/DejaVuSansMono.ttf)
set(Docs
    fonts/DejaVuSansMono.LICENSE
    fonts/NotoSansCJK.LICENSE
    ../README.md
    ../LICENSE.md)
set(ResourceFiles ${Fonts})

add_custom_target(resources ALL SOURCES ${ResourceFiles})

foreach(ResourceFile ${ResourceFiles})
  add_custom_command(TARGET resources PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                     copy ${CMAKE_SOURCE_DIR}/resources/${ResourceFile}
                          $<TARGET_FILE_DIR:libdecaf>/resources/${ResourceFile})
endforeach()

install(FILES ${Fonts} DESTINATION "${DECAF_INSTALL_RESOURCESDIR}/fonts")
install(FILES ${Docs} DESTINATION "${DECAF_INSTALL_DOCSDIR}")
