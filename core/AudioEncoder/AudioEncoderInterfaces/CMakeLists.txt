cmake_minimum_required(VERSION 3.1)

project(acsdkAudioEncoderInterfaces LANGUAGES CXX)
add_library(acsdkAudioEncoderInterfaces INTERFACE)
target_include_directories(acsdkAudioEncoderInterfaces INTERFACE "${acsdkAudioEncoderInterfaces_SOURCE_DIR}/include")
target_link_libraries(acsdkAudioEncoderInterfaces INTERFACE AVSCommon)

# install interface
asdk_install_interface()
