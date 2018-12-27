project(decaf-sdl)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

if(MSVC)
   set(RESOURCE_FILES
      ${CMAKE_SOURCE_DIR}/resources/decaf-sdl.rc
      hidpi.manifest)
else()
   set(RESOURCE_FILES "")
endif()

add_executable(decaf-sdl
   ${SOURCE_FILES}
   ${HEADER_FILES}
   ${RESOURCE_FILES})

target_include_directories(decaf-sdl PRIVATE
    ${SDL2_INCLUDE_DIRS})
target_link_libraries(decaf-sdl
    common
    common-sdl
    libconfig
    libdebugui
    libdecaf
    cpptoml
    excmd
    ${SDL2_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(decaf-sdl X11)
endif()

if(MSVC)
    set_target_properties(decaf-sdl PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

install(TARGETS decaf-sdl RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
