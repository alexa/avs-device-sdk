cmake_minimum_required(VERSION 3.1)

project(acsdkSystemClockMonitorInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkSystemClockMonitorInterfaces INTERFACE)

target_include_directories(acsdkSystemClockMonitorInterfaces INTERFACE
        "${acsdkSystemClockMonitorInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkSystemClockMonitorInterfaces INTERFACE
        acsdkNotifierInterfaces)

# install interface
asdk_install_interface()