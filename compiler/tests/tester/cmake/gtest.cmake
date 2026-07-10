# Search for the googletest library in the system and fetch if missing

include(FetchContent)

find_package(GTest REQUIRED)

if (NOT GTest_FOUND)
    message(STATUS "Fetching gtest...") 
    FetchContent_Declare(
        googletest
        URL ${GTEST_FETCH_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
else ()
    print_colored_message(
        "GTEST tests folder: ${GTEST_DIR}"
        ${STATUS_COLOR_MESSAGE} 
        ""
        STATUS
    )
endif()







# Take all google test files

file(GLOB GTEST_FILES "${GTEST_DIR}/*.cpp")

if(NOT GTEST_FILES)

    print_colored_message(
        "No test source files found in GTEST_DIR: ${GTEST_DIR}"
        ${FATAL_ERROR_COLOR_MESSAGE} 
        ""
        FATAL_ERROR
    )

endif()








# LLVM Coverage

set( LLVMCOV_COMPILE_OPTIONS
        "-fprofile-instr-generate"
        "-fcoverage-mapping" )

set( LLVMCOV_LINK_OPTIONS "-fprofile-instr-generate" )


function(add_coverage_to_target target_name)
    if(TARGET ${target_name})
        message(STATUS "Adding LLVM coverage to: ${target_name}")

        get_target_property(target_type ${target_name} TYPE)

        if (target_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${target_name} INTERFACE ${LLVMCOV_COMPILE_OPTIONS})
            target_link_options(${target_name} INTERFACE ${LLVMCOV_LINK_OPTIONS})
        else()
            target_compile_options(${target_name} PRIVATE ${LLVMCOV_COMPILE_OPTIONS})
            target_link_options(${target_name} PRIVATE ${LLVMCOV_LINK_OPTIONS})
        endif()
    else()
        print_colored_message(
            "Target ${target_name} does not exist"
            ${WARNING_COLOR_MESSAGE} 
            ""
            WARNING
        )
    endif()
endfunction()

function(add_coverage_to_targets targets_names)
    if(NOT "${targets_names}" STREQUAL "")
        foreach(target_name ${targets_names})
            if(TARGET ${target_name})
                add_coverage_to_target(${target_name})
            endif()
        endforeach()
    else()
        print_colored_message(
            "List of target names for llvm covereging is empty."
            ${WARNING_COLOR_MESSAGE} 
            ""
            WARNING
        )
    endif()
endfunction()

# Add coverage to all user additional libsw

if (LLVMCOV)
    if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        if (GTEST_ADDITIONAL_LIBS_NAMES_ENABLE_LLVMCOV)
            add_coverage_to_targets("${GTEST_ADDITIONAL_LIBS_NAMES_ENABLE_LLVMCOV}")
        else()
            add_coverage_to_targets("${GTEST_ADDITIONAL_LIBS_NAMES}")
        endif()
    else()
        print_colored_message(
            "Unable to turn on LLVM Coverage, compiler(${CMAKE_CXX_COMPILER_ID}) does not support it."
            ${WARNING_COLOR_MESSAGE} 
            ""
            WARNING
        )
    endif()
endif()









add_executable(${GTEST_OUTPUT_EXEC_FILE_NAME} ${GTEST_FILES})

target_link_libraries(${GTEST_OUTPUT_EXEC_FILE_NAME}
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${GTEST_ADDITIONAL_LIBS_NAMES}
)

target_compile_definitions(${GTEST_OUTPUT_EXEC_FILE_NAME} PRIVATE
    EXE_PATH="${CMAKE_BINARY_DIR}/paracl"
    TESTS_DIR="${CMAKE_SOURCE_DIR}"
)

target_compile_features(${GTEST_OUTPUT_EXEC_FILE_NAME} PRIVATE cxx_std_23)

add_coverage_to_target(${GTEST_OUTPUT_EXEC_FILE_NAME})





if (LLVMCOV)
    find_program(LLVM_PROFDATA llvm-profdata)
    find_program(LLVM_COV llvm-cov)

    if(NOT LLVM_PROFDATA OR NOT LLVM_COV)
        print_colored_message(
            "llvm-profdata not found! Coverage reports will be unavailable."
            "${WARNING_ERROR_COLOR_MESSAGE}" 
            "" 
            "WARNING"
        )
    endif()

    message("${CMAKE_BINARY_DIR}${TESTER_RELATIVE_PATH}/raw-%p.profraw")

    add_custom_command(
        OUTPUT ${GTEST_COV_HTML_DIR}/index.html
        
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}${TESTER_RELATIVE_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GTEST_COV_HTML_DIR}"
        
        COMMAND /usr/bin/env bash -c "rm -f ${CMAKE_BINARY_DIR}${TESTER_RELATIVE_PATH}/*.profraw"
        
        COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}${TESTER_RELATIVE_PATH}/raw-%p.profraw $<TARGET_FILE:${GTEST_OUTPUT_EXEC_FILE_NAME}> || true
        
        COMMAND /usr/bin/env bash -c "${LLVM_PROFDATA} merge -sparse ${CMAKE_BINARY_DIR}${TESTER_RELATIVE_PATH}/*.profraw -o ${GTEST_COV_DATA_FILE}"
        
        COMMAND ${LLVM_COV} show $<TARGET_FILE:${GTEST_OUTPUT_EXEC_FILE_NAME}> 
                -instr-profile=${GTEST_COV_DATA_FILE} 
                -format=html 
                -output-dir=${GTEST_COV_HTML_DIR} 
                "-ignore-filename-regex=${GTEST_FILES_AND_FOLDERS_IGNORED_BY_LLVMCOV}"
        
        DEPENDS ${GTEST_OUTPUT_EXEC_FILE_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Cleaning old data and generating fresh coverage report..."
        VERBATIM
    )

    add_custom_target(coverage
        DEPENDS ${GTEST_COV_HTML_DIR}/index.html
        COMMAND ${CMAKE_COMMAND} -E echo "Open coverage report: ${CMAKE_CURRENT_BINARY_DIR}/coverage-report/index.html"
        COMMENT "Generating coverage report..."
        VERBATIM
    )
endif()



