cmake_minimum_required(VERSION 3.1)
project(acsdkAlexaSeekControllerInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaSeekControllerInterfaces INTERFACE)

target_include_directories(acsdkAlexaSeekControllerInterfaces INTERFACE
        "${acsdkAlexaSeekControllerInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaSeekControllerInterfaces INTERFACE
        AVSCommon)

# install interface
asdk_install_interface()