project(latte-assembler)

include_directories(".")
include_directories(${PROJECT_BINARY_DIR})

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
file(GLOB_RECURSE HEADER_FILES src/*.h)

# Generates a grammar.h with the contents of grammar.txt in a C++ string
set(GRAMMAR_FILE ${PROJECT_SOURCE_DIR}/resources/grammar.txt)
set(GRAMMAR_CMAKE ${PROJECT_BINARY_DIR}/generate_grammar.cmake)

file(WRITE ${GRAMMAR_CMAKE} "FILE(WRITE grammar.h \"const char *LatteGrammar = R\\\"(\\n\")\n")
file(APPEND ${GRAMMAR_CMAKE} "FILE(READ ")
file(APPEND ${GRAMMAR_CMAKE} ${GRAMMAR_FILE})
file(APPEND ${GRAMMAR_CMAKE} " CONTENTS)\n")
file(APPEND ${GRAMMAR_CMAKE} "FILE(APPEND grammar.h \${CONTENTS})\n")
file(APPEND ${GRAMMAR_CMAKE} "FILE(APPEND grammar.h \")\\\";\\n\")\n")

add_custom_target(latte-assembler-grammar
    COMMAND ${CMAKE_COMMAND} -P ${GRAMMAR_CMAKE}
    ADD_DEPENDENCIES ${GRAMMAR_FILE})

# Build latte-assembler
add_executable(latte-assembler ${SOURCE_FILES} ${HEADER_FILES} ${GRAMMAR_FILE})
add_dependencies(latte-assembler latte-assembler-grammar)

target_link_libraries(latte-assembler
    common
    libgfd
    libgpu
    ${EXCMD_LIBRARIES}
    ${PEGLIB_LIBRARIES})

set_target_properties(latte-assembler-grammar PROPERTIES FOLDER tools)
set_target_properties(latte-assembler PROPERTIES FOLDER tools)
GroupSources(src)
GroupSources(resources)

install(TARGETS latte-assembler RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
