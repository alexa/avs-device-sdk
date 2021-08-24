cmake_minimum_required(VERSION 3.1)
project(acsdkAuthorizationInterfaces LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(acsdkAuthorizationInterfaces INTERFACE)

target_include_directories(acsdkAuthorizationInterfaces INTERFACE
    "${acsdkAuthorizationInterfaces_SOURCE_DIR}/include"
)

target_link_libraries(acsdkAuthorizationInterfaces INTERFACE)

# install interface
asdk_install_interface()
