cmake_minimum_required(VERSION 3.0)
project(DeviceSetup LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_subdirectory("acsdkDeviceSetupInterfaces")
add_subdirectory("acsdkDeviceSetup")
