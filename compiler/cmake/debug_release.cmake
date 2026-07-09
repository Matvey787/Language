if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()




message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:   ${CMAKE_CXX_COMPILER_ID}")




# Function adds compiler warnings
function(add_compiler_warnings)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(
            -Wall
            -Wextra
            -Wpedantic
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(
            /W4
            /WX
            /permissive-
            /utf-8
        )
    else()
        message(STATUS "No compiler warnings for compiler: "
                "${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()




# Function adds debug compiler debug options: sanitizer, implicit casts, e.t.c.
function(add_compiler_debug_options)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

        add_compile_options(
            -O0
            -g3
            -ggdb

            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls

            -fsanitize=address
            -fsanitize=undefined

            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wundef
            -Wunreachable-code
        )

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        add_compile_options(
            /Od  
            /Zi  
            /RTC1
            /sdl 
            /GS  
            /MDd 
        )
    else()

        message(STATUS "No debug options for compiler: ${CMAKE_CXX_COMPILER_ID}")

    endif()
    
endfunction()




# Function adds debug linker option (sanitizer)
function(add_linker_debug_options)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

        add_link_options(
            -fsanitize=address
            -fsanitize=undefined
        )

    endif()

endfunction()




# Function adds release compiler options
function(add_compiler_release_options)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

        add_compile_options(
            -O2
        )

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        add_compile_options(
            /O2 
            /Ob3
            /Oi
            /Ot
            /MD
        )

    endif()

endfunction()





if(CMAKE_BUILD_TYPE STREQUAL "Debug")

    add_compiler_warnings()
    add_compiler_debug_options()
    add_linker_debug_options()
    add_compile_definitions(DEBUG)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")

    add_compiler_release_options()

else()

    message(WARNING "Unknown build type: ${CMAKE_BUILD_TYPE}")

endif()
