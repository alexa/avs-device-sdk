cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkPropertiesInterfaces LANGUAGES CXX)

add_library(acsdkPropertiesInterfaces INTERFACE)
target_include_directories(acsdkPropertiesInterfaces INTERFACE include)

# install target
asdk_install_interface()

add_subdirectory("test")
