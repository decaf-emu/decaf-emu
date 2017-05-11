project(tests)
include(ExternalProject)
include_directories("../src")

set(HLE_TEST_CONTENT_PATH_SRC "${PROJECT_SOURCE_DIR}/hle/content")
set(HLE_TEST_CONTENT_PATH_DST "${PROJECT_BINARY_DIR}/hle/content")

if(DECAF_BUILD_TESTS)
    add_subdirectory("cpu")
endif()

if(DECAF_BUILD_WUT_TESTS)
    add_custom_target(hle-content)
    add_custom_command(TARGET hle-content
                       COMMAND ${CMAKE_COMMAND} -E
                           copy ${HLE_TEST_CONTENT_PATH_SRC}/short_text.txt ${HLE_TEST_CONTENT_PATH_DST}/short_text.txt)
    set_target_properties(hle-content PROPERTIES FOLDER tests)

    if(DECAF_BUILD_TOOLS)
        # Assemble shaders for gx2 tests
        add_custom_command(TARGET hle-content
                           COMMAND ${CMAKE_COMMAND} -E
                               make_directory ${HLE_TEST_CONTENT_PATH_DST}/shaders)

        macro(assemble_shader target gsh psh vsh)
            add_custom_command(TARGET hle-content
                               COMMAND latte-assembler compile --psh ${psh} --vsh ${vsh} ${gsh}
                               ADD_DEPENDENCIES ${psh} ${vsh})
        endmacro()

        assemble_shader(shader_pos_colour
            ${HLE_TEST_CONTENT_PATH_DST}/shaders/pos_colour.gsh
            ${HLE_TEST_CONTENT_PATH_SRC}/shaders/pos_colour.psh
            ${HLE_TEST_CONTENT_PATH_SRC}/shaders/pos_colour.vsh)
    endif()

    externalproject_add(hle-rpx
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/hle"
        INSTALL_COMMAND ""
        CMAKE_GENERATOR "Unix Makefiles"
        CMAKE_CACHE_ARGS
            -DDEVKITPPC:string=${DEVKITPPC}
            -DWUT_ROOT:string=${WUT_ROOT}
            -DCMAKE_TOOLCHAIN_FILE:string=${WUT_ROOT}/cmake/wut-toolchain.cmake)
    externalproject_get_property(hle-rpx BINARY_DIR)
    set_target_properties(hle-rpx PROPERTIES FOLDER tests)
    add_dependencies(hle-rpx hle-content)

    install(DIRECTORY "${BINARY_DIR}/"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/hle"
            FILES_MATCHING
                PATTERN "coreinit/*.rpx"
                PATTERN "CMakeFiles" EXCLUDE)

    install(DIRECTORY "${HLE_TEST_CONTENT_PATH_DST}"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/tests/hle")

    # Add tests to CTests
    macro(add_coreinit_test source)
        get_filename_component(name ${source} NAME_WE)
        add_test(NAME tests_hle_coreinit_${name}
                 WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                 COMMAND decaf-cli play "${BINARY_DIR}/coreinit/${name}.rpx" --content-path "${HLE_TEST_CONTENT_PATH_DST}")
    endmacro()

    add_subdirectory("hle")
endif()
