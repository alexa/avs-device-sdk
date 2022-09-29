cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkAlexaPresentationInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaPresentationInterfaces INTERFACE)

target_include_directories(acsdkAlexaPresentationInterfaces INTERFACE include)

target_link_libraries(acsdkAlexaPresentationInterfaces INTERFACE acsdkAPLCapabilityCommonInterfaces)

add_subdirectory("test")

# install interface
asdk_install_interface()