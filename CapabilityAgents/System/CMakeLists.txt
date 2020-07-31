cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(AVSSystem LANGUAGES CXX)

add_subdirectory("src")
add_subdirectory("test")

if (A4B)
    add_subdirectory("a4btest")
endif()