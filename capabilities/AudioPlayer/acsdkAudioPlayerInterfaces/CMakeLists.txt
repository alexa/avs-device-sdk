cmake_minimum_required(VERSION 3.1)
project(acsdkAudioPlayerInterfaces LANGUAGES CXX)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_library(acsdkAudioPlayerInterfaces INTERFACE)

target_include_directories(acsdkAudioPlayerInterfaces INTERFACE
    "${acsdkAudioPlayerInterfaces_SOURCE_DIR}/include"
)

# install interface
asdk_install_interface()
