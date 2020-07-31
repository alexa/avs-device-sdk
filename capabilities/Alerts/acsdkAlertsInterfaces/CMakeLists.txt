cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkAlertsInterfaces LANGUAGES CXX)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_library(acsdkAlertsInterfaces INTERFACE)

target_include_directories(acsdkAlertsInterfaces INTERFACE include)

# install interface
asdk_install_interface()
