cmake_minimum_required(VERSION 3.1)
project(acsdkAlexaRecordControllerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaRecordControllerInterfaces INTERFACE)

target_include_directories(acsdkAlexaRecordControllerInterfaces INTERFACE
        "${acsdkAlexaRecordControllerInterfaces_SOURCE_DIR}/include")

# install interface
asdk_install_interface()
