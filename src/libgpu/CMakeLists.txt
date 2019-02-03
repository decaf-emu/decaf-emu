project(libgpu)

include_directories(".")
include_directories("src")
include_directories("vulkan_shaders")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)
file(GLOB_RECURSE INLINE_FILES *.inl)
file(GLOB_RECURSE VULKANSHADER_FILES vulkan_shaders/*)

if(DECAF_VULKAN)
    find_program(GLSLANG_VALIDATOR NAMES glslangValidator
                HINTS "${Vulkan_INCLUDE_DIR}/../Bin")
    find_program(SPIRV_OPT NAMES spirv-opt
                HINTS "${Vulkan_INCLUDE_DIR}/../Bin")

    set(GENERATED_BASEPATH "${PROJECT_BINARY_DIR}/generated")
    set(SHDRBIN_BASEPATH "${GENERATED_BASEPATH}/vulkan_shaders_bin")

    include_directories(${GENERATED_BASEPATH})

    macro(compile_vulkan_shader OUTFILE INFILE)
        set(BIN2C "${CMAKE_SOURCE_DIR}/libraries/bin2c.cmake")
        set(INPATH "${PROJECT_SOURCE_DIR}/vulkan_shaders/${INFILE}")
        set(OUTPATH "${SHDRBIN_BASEPATH}/${OUTFILE}")
        add_custom_command(
            OUTPUT "${OUTPATH}.h" "${OUTPATH}.cpp"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHDRBIN_BASEPATH}"
            COMMAND ${GLSLANG_VALIDATOR} -V -o "${OUTPATH}" "${INPATH}"
            COMMAND ${CMAKE_COMMAND} ARGS "-DOUTPUT_C=${OUTPATH}.cpp" "-DOUTPUT_H=${OUTPATH}.h" "-DINPUT_FILES=${OUTPATH}" -P "${BIN2C}"
            DEPENDS "${INPATH}" "${BIN2C}"
            VERBATIM)
        list(APPEND VK_BIN_FILES "${OUTPATH}.h")
        list(APPEND VK_BIN_FILES "${OUTPATH}.cpp")
        set_source_files_properties("${OUTPATH}.cpp" PROPERTIES GENERATED TRUE)
        set_source_files_properties("${OUTPATH}.h" PROPERTIES GENERATED TRUE)
    endmacro()

    # compile the main retiling shader itself
    compile_vulkan_shader("gpu7_tiling.comp.spv" "gpu7_tiling.comp.glsl")
    add_custom_target(libgpu-shaders DEPENDS ${VK_BIN_FILES})
endif()

add_library(libgpu STATIC ${SOURCE_FILES} ${HEADER_FILES} ${INLINE_FILES} ${VULKANSHADER_FILES} ${VK_BIN_FILES})
GroupSources(latte)
GroupSources(src)
GroupSources(vulkan_shaders)

# We have to do this manually as the files don't exist until the first
# build occurs, which is after the solution is generated.
if(DECAF_VULKAN)
    if (MSVC)
        add_dependencies(libgpu libgpu-shaders)
        source_group("vulkan_shaders_bin" FILES ${VK_BIN_FILES})
    endif()
endif()

target_link_libraries(libgpu
    common
    libcpu
    addrlib)

if(DECAF_GL)
    target_link_libraries(libgpu
        ${GLBINDING_LIBRARIES}
        ${OPENGL_LIBRARIES})

   if(MSVC)
       # GLbinding linker warning
       target_compile_options(libgpu PUBLIC /wd4251)
   endif()
endif()

if(DECAF_VULKAN)
    target_link_libraries(libgpu
        vulkan
        ${SPIRV_LIBRARIES})
endif()

enable_pch(libgpu)
