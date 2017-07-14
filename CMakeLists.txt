cmake_minimum_required(VERSION 3.6)
project(se)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c forward_types.h rope.h rope.c util.h util.c vector.h vector.c buf.h buf.c stack.c stack.h
        circular_buffer.c circular_buffer.h editor_buffer.h editor_buffer.c)

add_executable(se ${SOURCE_FILES})
