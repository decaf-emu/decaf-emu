project(common)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

if(MSVC)
    add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES} "common.natvis")
else()
    add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES})
endif()
GroupSources(src)

target_link_libraries(common
    ${CEREAL_LIBRARIES}
    ${FIXED_POINT_LIBRARIES}
    ${GSL_LIBRARIES}
    ${SPDLOG_LIBRARIES})

if(MSVC)
    target_link_libraries(common Dbghelp)
endif()
