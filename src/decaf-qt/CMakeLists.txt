project(decaf-qt)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

include_directories("src")

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
file(GLOB_RECURSE HEADER_FILES src/*.h)
file(GLOB_RECURSE UI_FILES ui/*.ui)
file(GLOB_RECURSE QRC_FILES resources/*.qrc)

if(MSVC)
   set(RESOURCE_FILES
      ${CMAKE_SOURCE_DIR}/resources/decaf-sdl.rc
      ${CMAKE_SOURCE_DIR}/resources/hidpi.manifest)
else()
   set(RESOURCE_FILES "")
endif()

qt5_wrap_ui(UIS_HDRS ${UI_FILES})
qt5_add_resources(QT_RESOURCES ${QRC_FILES})
if(MSVC)
    source_group("Generated Files" FILES ${UIS_HDRS})
endif()

add_executable(decaf-qt
   ${SOURCE_FILES}
   ${HEADER_FILES}
   ${UIS_HDRS}
   ${QT_RESOURCES}
   ${RESOURCE_FILES}
   ${QHEXVIEW_HEADERS}
   ${QHEXVIEW_SOURCES})
GroupSources("Source Files" src)
GroupSources("UI Files" ui)
GroupSources("Resource Files" resources)

target_include_directories(decaf-qt PRIVATE
   ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
   ${SDL2_INCLUDE_DIRS})

target_link_libraries(decaf-qt
   common
   libconfig
   libcpu
   libdecaf
   ${CPPTOML_LIBRARIES}
   ${EXCMD_LIBRARIES}
   ${SDL2_LIBRARIES}
   qtadvanceddocking
   Qt5::Core
   Qt5::Concurrent
   Qt5::Svg
   Qt5::Widgets
   Qt5::Xml)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3") # Qt disapproves

   target_link_libraries(decaf-qt
      Setupapi
      Qt5::WinMain)

   set_target_properties(decaf-qt PROPERTIES
      LINK_FLAGS "/SUBSYSTEM:WINDOWS")

   # Find windeployqt binary in the same directory as qmake
   if(TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
      get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

      execute_process(
         COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
         RESULT_VARIABLE return_code
         OUTPUT_VARIABLE qt5_install_prefix
         OUTPUT_STRIP_TRAILING_WHITESPACE)

      set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

      if(EXISTS ${imported_location})
         add_executable(Qt5::windeployqt IMPORTED)

         set_target_properties(Qt5::windeployqt PROPERTIES
            IMPORTED_LOCATION ${imported_location})
      endif()
   endif()

   # Windeployqt to the TARGET_FILE_DIR:decaf-qt
   add_custom_command(TARGET decaf-qt
      POST_BUILD
      COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
      COMMAND Qt5::windeployqt
         --dir "$<TARGET_FILE_DIR:decaf-qt>"
         "$<TARGET_FILE_DIR:decaf-qt>/$<TARGET_FILE_NAME:decaf-qt>"
         --no-angle --no-opengl-sw --no-translations)

   # Windeployqt to temporary directory which we can copy in install
   add_custom_command(TARGET decaf-qt
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
      COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
      COMMAND Qt5::windeployqt
         --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
         "$<TARGET_FILE_DIR:decaf-qt>/$<TARGET_FILE_NAME:decaf-qt>"
         --no-angle --no-opengl-sw --no-translations)

   # copy deployment directory during installation
   install(
       DIRECTORY
       "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
       DESTINATION ${DECAF_INSTALL_BINDIR})
endif()

install(TARGETS decaf-qt RUNTIME DESTINATION "${DECAF_INSTALL_BINDIR}")
