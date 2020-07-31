cmake_minimum_required(VERSION 3.1)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_library(acsdkBluetoothInterfacesTestUtils INTERFACE)
target_include_directories(acsdkBluetoothInterfacesTestUtils INTERFACE "${acsdkBluetoothInterfaces_SOURCE_DIR}/test/include")
