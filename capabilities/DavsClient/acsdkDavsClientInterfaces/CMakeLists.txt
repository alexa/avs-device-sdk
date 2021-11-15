cmake_minimum_required(VERSION 3.1)
project(acsdkDavsClientInterfaces LANGUAGES CXX)

if (DEFINED AVS_CMAKE_BUILD)
    include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)
endif()

add_library(acsdkDavsClientInterfaces INTERFACE)

target_include_directories(acsdkDavsClientInterfaces INTERFACE
    "${acsdkDavsClientInterfaces_SOURCE_DIR}/include"
)

# install interface
asdk_install_interface()
