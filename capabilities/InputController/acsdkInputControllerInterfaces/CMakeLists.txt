cmake_minimum_required(VERSION 3.0)
project(acsdkInputControllerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkInputControllerInterfaces INTERFACE)

target_include_directories(acsdkInputControllerInterfaces INTERFACE
        "${acsdkInputControllerInterfaces_SOURCE_DIR}/include")

# install interface
asdk_install_interface()