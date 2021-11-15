cmake_minimum_required(VERSION 3.1)

project(acsdkCommunication LANGUAGES CXX)

add_library(acsdkCommunication INTERFACE)

target_include_directories(acsdkCommunication INTERFACE
        "${acsdkCommunication_SOURCE_DIR}/include"
        )

target_link_libraries(acsdkCommunication INTERFACE acsdkCommunicationInterfaces acsdkNotifier)

# install interface
asdk_install_interface()

add_subdirectory("test")