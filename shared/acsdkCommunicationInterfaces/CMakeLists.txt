cmake_minimum_required(VERSION 3.1)

project(acsdkCommunicationInterfaces LANGUAGES CXX)

add_library(acsdkCommunicationInterfaces INTERFACE)

target_include_directories(acsdkCommunicationInterfaces INTERFACE
        "${acsdkCommunicationInterfaces_SOURCE_DIR}/include"
        )


# install interface
asdk_install_interface()