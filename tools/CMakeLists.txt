project(tools)
include(ExternalProject)
include_directories(".")
include_directories("../src")

add_subdirectory(gfd-tool)
add_subdirectory(latte-assembler)

if(DECAF_GL)
   if(DECAF_SDL)
       add_subdirectory(pm4-replay)
   endif()

   if(DECAF_QT)
       # add_subdirectory(pm4-replay-qt)
   endif()
endif()

if(DEVKITPPC AND WUT_ROOT)
   externalproject_add(wiiu-rpc
      SOURCE_DIR "${PROJECT_SOURCE_DIR}/wiiu-rpc"
      INSTALL_COMMAND ""
      CMAKE_GENERATOR "Unix Makefiles"
      CMAKE_CACHE_ARGS
         -DDEVKITPPC:string=${DEVKITPPC}
         -DWUT_ROOT:string=${WUT_ROOT}
         -DCMAKE_TOOLCHAIN_FILE:string=${WUT_ROOT}/share/wut.toolchain.cmake)
   set_target_properties(wiiu-rpc PROPERTIES FOLDER tools)

   externalproject_add_step(wiiu-rpc forcebuild
      COMMAND ${CMAKE_COMMAND} -E echo "Force build of wiiu-rpc"
      DEPENDEES "configure"
      DEPENDERS "build"
      ALWAYS 1)
endif()
