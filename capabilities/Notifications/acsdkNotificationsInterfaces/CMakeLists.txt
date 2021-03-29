cmake_minimum_required(VERSION 3.1)
project(acsdkNotificationsInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkNotificationsInterfaces INTERFACE)

target_include_directories(acsdkNotificationsInterfaces INTERFACE
        "${acsdkNotificationsInterfaces_SOURCE_DIR}/include")

# install interface
asdk_install_interface()