cmake_minimum_required(VERSION 3.9)
project(integrate_c)

set(CMAKE_CXX_STANDARD 11)

find_package (Threads REQUIRED)
include_directories(${CMAKE_THREAD_INCLUDE_DIR})

set(SOURCE_FILES
        main.c
        count.c
        count.h
        )

add_executable(integrate_c main.c count.c)
target_link_libraries (integrate_c ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(integrate_c m)