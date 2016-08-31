set(CMAKE_CXX_FLAGS "-Werror=shadow ${CMAKE_CXX_FLAGS}")

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES})
