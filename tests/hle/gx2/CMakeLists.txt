cmake_minimum_required(VERSION 3.2)
project(gx2)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT COMMAND add_gx2_test)
macro(add_gx2_test source)
   get_filename_component(name ${source} NAME_WE)
   add_rpx_lite(${name} ${source})
   target_link_libraries(${name} whb gfd defaultheap coreinit gx2 proc_ui)
endmacro()
endif()

add_gx2_test(draw/triangle.c)
