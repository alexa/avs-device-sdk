cmake_minimum_required(VERSION 3.1)

add_subdirectory("acsdkDavsClientInterfaces")

add_subdirectory("acsdkAssetsInterfaces")

if (ASSET_MANAGER)
    add_subdirectory("acsdkAssetsCommon")
    add_subdirectory("acsdkDavsClient")
endif()