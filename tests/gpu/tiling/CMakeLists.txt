include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

add_executable(test-gpu-tiling ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(test-gpu-tiling PROPERTIES FOLDER tests)

target_link_libraries(test-gpu-tiling
    addrlib
    catch2
    common
    libcpu
    libgpu)

if(DECAF_VULKAN)
    target_link_libraries(test-gpu-tiling
        vulkan
        ${SPIRV_LIBRARIES})
endif()

add_test(NAME gpu-tiling
         WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
         COMMAND test-gpu-tiling)
