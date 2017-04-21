project(tests)
include(ExternalProject)

include_directories("../src")
add_subdirectory("cpu")

if(DECAF_BUILD_WUT_TESTS)
   externalproject_add(hle-rpx
      SOURCE_DIR "${PROJECT_SOURCE_DIR}/hle"
      INSTALL_COMMAND ""
      CMAKE_GENERATOR "Unix Makefiles"
      CMAKE_CACHE_ARGS
         -DDEVKITPPC:string=${DEVKITPPC}
         -DWUT_ROOT:string=${WUT_ROOT}
         -DCMAKE_TOOLCHAIN_FILE:string=${WUT_ROOT}/cmake/wut-toolchain.cmake)
   externalproject_get_property(hle-rpx BINARY_DIR)
   set_target_properties(hle-rpx PROPERTIES FOLDER tests)

   install(DIRECTORY "${BINARY_DIR}/"
           DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/hle"
           FILES_MATCHING
               PATTERN "coreinit/*.rpx"
               PATTERN "CMakeFiles" EXCLUDE)

   install(DIRECTORY "${PROJECT_SOURCE_DIR}/hle/content"
           DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/hle")
endif()
