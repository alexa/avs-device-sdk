cmake_minimum_required(VERSION 3.1)

project(acsdkSampleApplicationInterfaces LANGUAGES CXX)

add_library(acsdkSampleApplicationInterfaces INTERFACE)

target_include_directories(acsdkSampleApplicationInterfaces INTERFACE
    "${acsdkSampleApplicationInterfaces_SOURCE_DIR}/include"
)

# install interface
asdk_install_interface()
