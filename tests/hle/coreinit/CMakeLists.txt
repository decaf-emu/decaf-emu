cmake_minimum_required(VERSION 3.2)
project(coreinit)
include(${WUT_ROOT}/share/wut.cmake REQUIRED)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT COMMAND add_coreinit_test)
macro(add_coreinit_test source)
   get_filename_component(name ${source} NAME_WE)
   add_executable(${name} ${source})
   target_link_libraries(${name} coreinit)
   wut_create_rpx(${name}.rpx ${name})
endmacro()
endif()

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

add_coreinit_test(messagequeue/messagequeue_high_priority.c)
add_coreinit_test(messagequeue/messagequeue_peek.c)
add_coreinit_test(messagequeue/messagequeue_peek_empty.c)
add_coreinit_test(messagequeue/messagequeue_receive_block.c)
add_coreinit_test(messagequeue/messagequeue_receive_empty.c)
add_coreinit_test(messagequeue/messagequeue_send_full_block.c)
add_coreinit_test(messagequeue/messagequeue_send_full_fail.c)
add_coreinit_test(messagequeue/messagequeue_send_receive.c)

add_coreinit_test(thread/thread_cancel.c)
