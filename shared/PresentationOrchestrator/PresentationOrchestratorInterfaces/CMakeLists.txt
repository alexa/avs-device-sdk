cmake_minimum_required(VERSION 3.1)
project(acsdkPresentationOrchestratorInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkPresentationOrchestratorInterfaces INTERFACE)

target_include_directories(acsdkPresentationOrchestratorInterfaces INTERFACE
        "${acsdkPresentationOrchestratorInterfaces_SOURCE_DIR}/include")

add_subdirectory("test")

asdk_install_interface()
