cmake_minimum_required(VERSION 3.2)
project(wiiu-rpc)
include(${WUT_ROOT}/share/wut.cmake REQUIRED)

set(CMAKE_C_STANDARD 99)

add_executable(wiiu-rpc
    src/main.c
    src/console.c
    src/server.c
    src/packet.c)

target_link_libraries(wiiu-rpc
    whb
    coreinit
    sysapp
    nsysnet
    nn_ac
    proc_ui)

wut_enable_newlib(wiiu-rpc)
wut_create_rpx(wiiu-rpc.rpx wiiu-rpc)
