project(decaf-sdl)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

if(MSVC)
   set(RESOURCE_FILES
      ${CMAKE_SOURCE_DIR}/resources/decaf-sdl.rc
      ${CMAKE_SOURCE_DIR}/resources/hidpi.manifest)
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
    libconfig
    libdecaf
    cpptoml
    excmd
    ${SDL2_LIBRARIES})

if(MSVC)
    set_target_properties(decaf-sdl PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS")
    target_link_libraries(decaf-sdl Setupapi)
endif()

if(DECAF_PCH)
    target_precompile_headers(decaf-sdl
      PRIVATE
        <SDL.h>
    )

    AutoGroupPCHFiles()
endif()

install(TARGETS decaf-sdl RUNTIME DESTINATION "${DECAF_INSTALL_BINDIR}")
