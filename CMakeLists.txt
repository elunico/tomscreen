cmake_minimum_required(VERSION 3.13)

project(tomscreen)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
option(NDEBUG on)
if (NDEBUG)
set(CMAKE_C_FLAGS "-O3 ${CMAKE_C_FLAGS}")
else()
set(CMAKE_C_FLAGS "-g -O0 -fsanitize=address -fsanitize=undefined ${CMAKE_C_FLAGS}")
endif()
add_library(tomscreen screen.c)
