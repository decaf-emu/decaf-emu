cmake_minimum_required(VERSION 3.2)
project(tests-hle)

include_directories(${CMAKE_SOURCE_DIR}/common)
add_subdirectory(coreinit)
add_subdirectory(gx2)
