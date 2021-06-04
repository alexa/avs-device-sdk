cmake_minimum_required(VERSION 3.0)
project(acsdkDeviceSetupInterfaces LANGUAGES CXX)

add_library(acsdkDeviceSetupInterfaces INTERFACE)

target_include_directories(acsdkDeviceSetupInterfaces INTERFACE "${acsdkDeviceSetupInterfaces_SOURCE_DIR}/include")

# install target
asdk_install_interface()
