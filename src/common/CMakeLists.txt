project(common)

include_directories(".")

set(BUILD_VERSION "0")
if ($ENV{CI})
  if ($ENV{TRAVIS})
    set(BUILD_TAG $ENV{TRAVIS_TAG})
  elseif($ENV{APPVEYOR})
    set(BUILD_TAG $ENV{APPVEYOR_REPO_TAG_NAME})
  endif()

  if (BUILD_TAG)
    string(REGEX MATCH "${CMAKE_MATCH_1}-([0-9]+)" OUTVAR ${BUILD_TAG})
    if (${CMAKE_MATCH_COUNT} GREATER 0)
	  set(BUILD_VERSION ${CMAKE_MATCH_1})
    endif()
  endif()
endif()
  
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/scm_rev.cpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/scm_rev.cpp" @ONLY)


file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_library(common STATIC ${SOURCE_FILES} ${HEADER_FILES})
GroupSources(src)

target_link_libraries(common
    fixed_point
    fmt::fmt
    gsl
    spdlog)

if(MSVC)
    target_link_libraries(common Dbghelp)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(common rt)
endif()
