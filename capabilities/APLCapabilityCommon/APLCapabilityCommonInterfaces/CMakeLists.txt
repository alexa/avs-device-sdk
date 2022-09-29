cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkAPLCapabilityCommonInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAPLCapabilityCommonInterfaces INTERFACE)

target_include_directories(acsdkAPLCapabilityCommonInterfaces INTERFACE include)
target_link_libraries(acsdkAPLCapabilityCommonInterfaces INTERFACE acsdkPresentationOrchestratorInterfaces)

add_subdirectory("test")

# install interface
asdk_install_interface()