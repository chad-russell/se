cmake_minimum_required(VERSION 3.6)
project(se)

set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

set(SOURCE_FILES forward_types.h rope.h rope.c util.h util.c vector.h vector.c buf.h buf.c stack.c stack.h
        circular_buffer.c circular_buffer.h editor_buffer.h editor_buffer.c line_rope.h line_rope.c)

add_executable(se_test main.c ${SOURCE_FILES})

add_library(se STATIC ${SOURCE_FILES})