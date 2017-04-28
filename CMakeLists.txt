cmake_minimum_required(VERSION 3.6)
project(se)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c forward_types.h rope.h rope.c util.h util.c vector.h vector.c buf.h buf.c stack.c stack.h circular_buffer.c circular_buffer.h json_tokenize.c json_tokenize.h)
add_executable(se ${SOURCE_FILES})

# path to libczmq.a
set(CZMQ_LIB_PATH /usr/local/lib)

# path to czmq.h
set(CZMQ_INCLUDE_PATH /usr/local/include)

find_library(ZMQ zmq paths ${ZMQ_LIB_PATH})
find_library(CZMQ czmq paths ${ZMQ_LIB_PATH})

include_directories(${CZMQ_INCLUDE_PATH})
target_link_libraries(se ${ZMQ} ${CZMQ})
