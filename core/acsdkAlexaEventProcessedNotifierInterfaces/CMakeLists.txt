cmake_minimum_required(VERSION 3.1)

project(acsdkAlexaEventProcessedNotifierInterfaces LANGUAGES CXX)

add_library(acsdkAlexaEventProcessedNotifierInterfaces INTERFACE)

target_include_directories(acsdkAlexaEventProcessedNotifierInterfaces INTERFACE
        "${acsdkAlexaEventProcessedNotifierInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaEventProcessedNotifierInterfaces INTERFACE
        AVSCommon
        acsdkNotifierInterfaces)

# install interface
asdk_install_interface()
