cmake_minimum_required(VERSION 3.1)

project(acsdkNotifierInterfaces LANGUAGES CXX)

add_library(acsdkNotifierInterfaces INTERFACE)

target_include_directories(acsdkNotifierInterfaces INTERFACE
    "${acsdkNotifierInterfaces_SOURCE_DIR}/include"
)

add_subdirectory("test")

# install interface
asdk_install_interface()
