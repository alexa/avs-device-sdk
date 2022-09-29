cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkAlexaRemoteVideoPlayerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaRemoteVideoPlayerInterfaces INTERFACE)

target_include_directories(acsdkAlexaRemoteVideoPlayerInterfaces INTERFACE
        "${acsdkAlexaRemoteVideoPlayerInterfaces_SOURCE_DIR}/include"
        "${AlexaVideoCommon}/include")

# install interface
asdk_install_interface()
