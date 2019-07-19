cmake_minimum_required(VERSION 3.1)
project(BluetoothImplementations LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

if(BLUETOOTH_BLUEZ)
    add_subdirectory("BlueZ")
endif()
