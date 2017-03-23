cmake_minimum_required(VERSION 3.2)
project(coreinit)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

macro(add_coreinit_test source)
   get_filename_component(name ${source} NAME_WE)
   add_rpx(${name} ${source})
   target_link_libraries(${name} coreinit)
   install(FILES $<TARGET_FILE:${name}>.rpx DESTINATION "bin")
endmacro()

include_directories(${CMAKE_SOURCE_DIR}/common)

add_coreinit_test(alarm/alarm_cancel.c)
add_coreinit_test(alarm/alarm_cancel_group.c)
add_coreinit_test(alarm/alarm_multithread.c)
add_coreinit_test(alarm/alarm_periodic.c)
add_coreinit_test(alarm/alarm_simple.c)
add_coreinit_test(alarm/alarm_user_data.c)

add_coreinit_test(coroutine/coroutine_multi.c)
add_coreinit_test(coroutine/coroutine_single.c)

add_coreinit_test(filesystem/filesystem_read.c)

add_coreinit_test(memory/blockheap_simple.c)
add_coreinit_test(memory/frameheap_multi.c)
add_coreinit_test(memory/frameheap_simple.c)
add_coreinit_test(memory/frameheap_unaligned.c)

add_coreinit_test(thread/thread_cancel.c)
