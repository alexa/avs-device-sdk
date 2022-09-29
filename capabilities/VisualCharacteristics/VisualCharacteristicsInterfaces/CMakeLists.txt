cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkVisualCharacteristicsInterfaces LANGUAGES CXX)

add_library(acsdkVisualCharacteristicsInterfaces INTERFACE)

target_include_directories(acsdkVisualCharacteristicsInterfaces INTERFACE "${acsdkVisualCharacteristicsInterfaces_SOURCE_DIR}/include")

# install target
asdk_install_interface()
