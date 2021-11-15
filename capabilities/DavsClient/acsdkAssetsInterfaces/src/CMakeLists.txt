cmake_minimum_required(VERSION 3.1)

add_library(acsdkAssetsInterfaces
    DavsRequest.cpp
    UrlRequest.cpp
    VendableArtifact.cpp
    )

target_include_directories(acsdkAssetsInterfaces PUBLIC
    "${acsdkAssetsInterfaces_SOURCE_DIR}/include"
    "${RAPIDJSON_INCLUDE_DIR}"
    )

target_link_libraries(acsdkAssetsInterfaces
    AVSCommon
    acsdkNotifier
    acsdkCommunicationInterfaces
    acsdkCommunication
    )

# install target
asdk_install()