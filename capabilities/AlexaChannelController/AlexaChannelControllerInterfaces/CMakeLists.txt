cmake_minimum_required(VERSION 3.1)
project(acsdkAlexaChannelControllerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaChannelControllerInterfaces INTERFACE)

target_include_directories(acsdkAlexaChannelControllerInterfaces INTERFACE
        "${acsdkAlexaChannelControllerInterfaces_SOURCE_DIR}/include")

# install interface
asdk_install_interface()
