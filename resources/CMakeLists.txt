include(GNUInstallDirs)

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
set(PARENT_PROJECT_NAME decaf-emu)

set(ResourceFiles ${Fonts})

add_custom_target(resources ALL SOURCES ${ResourceFiles})

foreach(ResourceFile ${ResourceFiles})
  add_custom_command(TARGET resources PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                     copy ${CMAKE_SOURCE_DIR}/resources/${ResourceFile}
                          $<TARGET_FILE_DIR:libdecaf>/resources/${ResourceFile})
endforeach()

if(DECAF_RESOURCES_PATH)
  install(FILES ${Fonts} DESTINATION "${DECAF_RESOURCES_PATH}")
else()
  install(FILES ${Fonts}
          DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${PARENT_PROJECT_NAME}/resources/fonts")
endif()

if(DECAF_DOCS)
  install(FILES ${Docs} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${PARENT_PROJECT_NAME}")
endif()
