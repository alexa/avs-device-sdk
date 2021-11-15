cmake_minimum_required(VERSION 3.1)
project(acsdkKWDInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkKWDInterfaces INTERFACE)

target_include_directories(acsdkKWDInterfaces INTERFACE "${acsdkKWDInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkKWDInterfaces INTERFACE acsdkNotifierInterfaces)

# install interface
asdk_install_interface()
