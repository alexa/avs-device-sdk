cmake_minimum_required(VERSION 3.1)
project(acsdkAlexaVideoRecorderInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaVideoRecorderInterfaces INTERFACE)

target_include_directories(acsdkAlexaVideoRecorderInterfaces INTERFACE
        "${acsdkAlexaVideoRecorderInterfaces_SOURCE_DIR}/include"
        "${AlexaVideoCommon}/include")

# install interface
asdk_install_interface()
