cmake_minimum_required(VERSION 3.30)

project(ParaCL LANGUAGES CXX C VERSION 0.1)

find_package(BISON 3.2 REQUIRED)
find_package(FLEX REQUIRED)

find_package(LLVM REQUIRED CONFIG)
if(LLVM_VERSION_MAJOR LESS 21)
    message(FATAL_ERROR "LLVM 21 or higher is required!")
endif()

find_package(spdlog REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(EXE_NAME paracl)
set(FRONT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/front)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(THIRD_PARTY ${FRONT_DIR}/third_party)

file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
    OUTPUT ${GENERATED_DIR}/parser.tab.cc ${GENERATED_DIR}/parser.tab.hh
    COMMAND ${BISON_EXECUTABLE} -v -d -o ${GENERATED_DIR}/parser.tab.cc ${FRONT_DIR}/src/parser.y
    DEPENDS ${FRONT_DIR}/src/parser.y
)

add_custom_command(
    OUTPUT ${GENERATED_DIR}/lex.yy.cc
    COMMAND ${FLEX_EXECUTABLE} -o ${GENERATED_DIR}/lex.yy.cc ${FRONT_DIR}/src/lexer.l
    DEPENDS ${FRONT_DIR}/src/lexer.l ${GENERATED_DIR}/parser.tab.hh
)

file(GLOB_RECURSE AST_MODULES "${FRONT_DIR}/ast/*.cppm")

add_library(ast_lib STATIC ${AST_MODULES})

target_sources(ast_lib
    PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES ${AST_MODULES}
)







set(IR_GENERATOR_LIB ir_generator_lib)

file(GLOB_RECURSE IRGENERATOR_MODULES "${FRONT_DIR}/src/llvm/*.cppm")

add_library(${IR_GENERATOR_LIB} STATIC ${IRGENERATOR_MODULES})

target_sources(${IR_GENERATOR_LIB}
    PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES ${IRGENERATOR_MODULES}
)

target_link_libraries(${IR_GENERATOR_LIB} PRIVATE ast_lib LLVM spdlog::spdlog)
target_include_directories(${IR_GENERATOR_LIB} PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(${IR_GENERATOR_LIB} PUBLIC ${LLVM_DEFINITIONS})
target_compile_options(${IR_GENERATOR_LIB} PRIVATE -g -O0)
target_compile_features(${IR_GENERATOR_LIB} PRIVATE cxx_std_23)






target_compile_options(ast_lib PRIVATE -g -O0)
target_compile_features(ast_lib PRIVATE cxx_std_23)
target_include_directories(ast_lib PRIVATE ${THIRD_PARTY})





add_library(parser_lib STATIC
    ${GENERATED_DIR}/parser.tab.cc
)

set_source_files_properties(
    ${GENERATED_DIR}/parser.tab.cc
    PROPERTIES GENERATED TRUE
)

file(GLOB_RECURSE PARSER_CONTEXT "${FRONT_DIR}/src/*.cppm")
list(FILTER PARSER_CONTEXT EXCLUDE REGEX ".*/llvm/.*")

target_sources(parser_lib
    PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES ${PARSER_CONTEXT}
)

target_include_directories(parser_lib PUBLIC ${GENERATED_DIR} ${THIRD_PARTY})
target_compile_features(parser_lib PRIVATE cxx_std_23)
target_link_libraries(parser_lib PUBLIC ast_lib)

add_library(lexer_lib STATIC
    ${GENERATED_DIR}/lex.yy.cc
)

set_source_files_properties(
    ${GENERATED_DIR}/lex.yy.cc
    PROPERTIES GENERATED TRUE
)

target_include_directories(lexer_lib PRIVATE ${GENERATED_DIR} ${THIRD_PARTY})
target_compile_features(lexer_lib PRIVATE cxx_std_23)
target_link_libraries(lexer_lib PRIVATE parser_lib)


add_executable(${EXE_NAME} ${FRONT_DIR}/src/main.cpp)

target_link_libraries(${EXE_NAME} PRIVATE parser_lib lexer_lib ast_lib ir_generator_lib)
target_include_directories(${EXE_NAME} PRIVATE ${THIRD_PARTY} ${GENERATED_DIR} ${FRONT_DIR}/src)
target_compile_features(${EXE_NAME} PRIVATE cxx_std_23)
target_compile_options(${EXE_NAME} PRIVATE -g -O0)