project(common-sdl)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(common-sdl STATIC ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(common-sdl PUBLIC
   ${SDL2_INCLUDE_DIRS})
target_link_libraries(common-sdl
    common
    libconfig
    libdebugui
    libdecaf
    cpptoml
    excmd
    ${SDL2_LIBRARIES})

if(UNIX)
    target_link_libraries(common-sdl dl)
endif()
