cmake_minimum_required(VERSION 3.30)

project(ParaCL
    LANGUAGES CXX
    VERSION 0.1
)


set(EXE_NAME paracl)
set(THIRD_PARTY third_party/)


add_executable(${EXE_NAME} src/main.cpp)

target_compile_features(${EXE_NAME} PRIVATE cxx_std_23)


file(GLOB_RECURSE AST_MODULES
    "${CMAKE_SOURCE_DIR}/ast/*.cppm"
)

target_sources(${EXE_NAME}
    PUBLIC
        FILE_SET
            CXX_MODULES
        TYPE
            CXX_MODULES
        FILES
            ${AST_MODULES}
)

target_include_directories(${EXE_NAME} PRIVATE ${THIRD_PARTY})
