cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkAlexaLiveViewControllerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaLiveViewControllerInterfaces INTERFACE)

target_include_directories(acsdkAlexaLiveViewControllerInterfaces INTERFACE
        "${acsdkAlexaLiveViewControllerInterfaces_SOURCE_DIR}/include")

# install interface
asdk_install_interface()