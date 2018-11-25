cmake_minimum_required(VERSION 3.2)
project(gx2)
include(${WUT_ROOT}/share/wut.cmake REQUIRED)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT COMMAND add_gx2_test)
macro(add_gx2_test source)
   get_filename_component(name ${source} NAME_WE)
   add_executable(${name} ${source})
   target_link_libraries(${name} whb gfd coreinit gx2 proc_ui sysapp nsysnet)
   wut_create_rpx(${name}.rpx ${name})
endmacro()
endif()

add_gx2_test(draw/triangle.c)
