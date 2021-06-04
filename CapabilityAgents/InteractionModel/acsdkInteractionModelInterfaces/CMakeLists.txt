cmake_minimum_required(VERSION 3.1)
project(acsdkInteractionModelInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkInteractionModelInterfaces INTERFACE)

target_include_directories(acsdkInteractionModelInterfaces INTERFACE
        "${acsdkInteractionModelInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkInteractionModelInterfaces INTERFACE
        acsdkNotifierInterfaces)

# install interface
asdk_install_interface()