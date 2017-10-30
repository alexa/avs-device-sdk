cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(TemplateRuntime LANGUAGES CXX)

add_subdirectory("src")
acsdk_add_test_subdirectory_if_allowed()
