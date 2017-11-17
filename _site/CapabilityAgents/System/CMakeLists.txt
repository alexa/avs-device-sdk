cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(AVSSystem LANGUAGES CXX)

add_subdirectory("src")
acsdk_add_test_subdirectory_if_allowed()
