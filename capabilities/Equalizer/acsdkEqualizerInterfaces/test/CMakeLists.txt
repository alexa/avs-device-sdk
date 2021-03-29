cmake_minimum_required(VERSION 3.1)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkEqualizerInterfacesTestUtils INTERFACE)

target_include_directories(acsdkEqualizerInterfacesTestUtils INTERFACE
    "${acsdkEqualizerInterfaces_SOURCE_DIR}/test/include")