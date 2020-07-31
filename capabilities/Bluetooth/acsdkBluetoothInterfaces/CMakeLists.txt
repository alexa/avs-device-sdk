cmake_minimum_required(VERSION 3.1)
project(acsdkBluetoothInterfaces LANGUAGES CXX)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_library(acsdkBluetoothInterfaces INTERFACE)

target_include_directories(acsdkBluetoothInterfaces INTERFACE
    "${acsdkBluetoothInterfaces_SOURCE_DIR}/include"
)

add_subdirectory("test")

# install interface
asdk_install_interface()