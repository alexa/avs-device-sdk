cmake_minimum_required(VERSION 3.1)

add_subdirectory("acsdkAssetManagerClientInterfaces")

if (ASSET_MANAGER)
    add_subdirectory("acsdkAssetManagerClient")
    add_subdirectory("acsdkAssetManager")
endif()
