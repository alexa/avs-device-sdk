cmake_minimum_required(VERSION 3.1)
project(BluetoothImplementationsBlueZ LANGUAGES CXX)

include(../../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
