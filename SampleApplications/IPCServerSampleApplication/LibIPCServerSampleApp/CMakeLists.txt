cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(LibIPCServerSampleApp LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)
include(${AVS_CMAKE_BUILD}/cmake/PrepareInstall.cmake)

add_subdirectory("src")
add_subdirectory("test")