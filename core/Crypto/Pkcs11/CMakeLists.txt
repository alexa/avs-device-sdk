cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkPkcs11 LANGUAGES CXX)

add_subdirectory("src")
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
    add_subdirectory("testStubs")
    add_subdirectory("test")
endif()
