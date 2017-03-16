project(resources)

set(Fonts
   fonts/DejaVuSansMono.ttf
   fonts/SourceSansPro-Regular.ttf
   fonts/LICENSE.txt)

set(ResourceFiles ${Fonts})

add_custom_target(resources ALL SOURCES ${ResourceFiles})

foreach(ResourceFile ${ResourceFiles})
  add_custom_command(TARGET resources PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                     copy ${CMAKE_SOURCE_DIR}/resources/${ResourceFile}
                          $<TARGET_FILE_DIR:libdecaf>/resources/${ResourceFile})
endforeach()

install(FILES ${Fonts}
        DESTINATION "${CMAKE_INSTALL_PREFIX}/resources/fonts")
