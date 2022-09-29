cmake_minimum_required(VERSION 3.1)
project(acsdkAlexaLauncherInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAlexaLauncherInterfaces INTERFACE)

target_include_directories(acsdkAlexaLauncherInterfaces INTERFACE
        "${acsdkAlexaLauncherInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaLauncherInterfaces INTERFACE
        AVSCommon)

# install interface
asdk_install_interface()