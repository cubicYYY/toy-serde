cmake_minimum_required (VERSION 3.16)
project (ToySerde
  VERSION 0.1.0
  DESCRIPTION "YYY's Personal Project (2023-OOP Final Project)"
  LANGUAGES CXX
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fconcepts")
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_BUILD_TYPE Debug)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

add_compile_options(-Wall -Wextra -pedantic -g)

include_directories(thirdparty src)
add_executable(main src/main.cpp)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # require at least gcc 10.3.0
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.3.0)
        message(FATAL_ERROR "GCC version must be at least 10.3.0!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler " ${CMAKE_CXX_COMPILER_ID} "! Compilation has only been tested with Clang and GCC.")
endif()