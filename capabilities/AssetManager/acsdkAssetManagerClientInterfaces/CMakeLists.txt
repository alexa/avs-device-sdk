cmake_minimum_required(VERSION 3.1)
project(acsdkAssetManagerClientInterfaces LANGUAGES CXX)

if (DEFINED AVS_CMAKE_BUILD)
    include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)
endif ()
add_library(acsdkAssetManagerClientInterfaces INTERFACE)

target_include_directories(acsdkAssetManagerClientInterfaces INTERFACE
    "${acsdkAssetManagerClientInterfaces_SOURCE_DIR}/include"
)

# install interface
asdk_install_interface()
