cmake_minimum_required(VERSION 3.1)
project(acsdkEqualizerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkEqualizerInterfaces INTERFACE)

target_include_directories(acsdkEqualizerInterfaces INTERFACE
    "${acsdkEqualizerInterfaces_SOURCE_DIR}/include"
)

add_subdirectory("test")

# install interface
asdk_install_interface()
