cmake_minimum_required(VERSION 3.2)
project(wiiu-rpc)

set(CMAKE_C_STANDARD 99)

add_rpx_lite(wiiu-rpc
    src/main.c
    src/console.c
    src/server.c
    src/packet.c)

target_link_libraries(wiiu-rpc
    whb
    defaultheap
    coreinit
    sysapp
    nsysnet
    vpad
    nn_ac)
