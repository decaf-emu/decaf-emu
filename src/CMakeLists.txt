include_directories(".")

add_subdirectory(common)
add_subdirectory(libconfig)
add_subdirectory(libcpu)
add_subdirectory(libdecaf)
add_subdirectory(libgfd)
add_subdirectory(libgpu)

add_subdirectory(decaf-cli)
add_subdirectory(decaf-sdl)

if(DECAF_QT)
    add_subdirectory(decaf-qt)
endif()
