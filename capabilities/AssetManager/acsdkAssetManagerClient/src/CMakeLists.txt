add_definitions("-DACSDK_LOG_MODULE=AssetManager")

add_library(acsdkAssetManagerClient
    ArtifactWrapper.cpp
    ArtifactWrapperFactory.cpp
    GenericInventory.cpp
    )

target_include_directories(acsdkAssetManagerClient PUBLIC
    "${acsdkAssetManagerClient_SOURCE_DIR}/include"
    )

target_link_libraries(acsdkAssetManagerClient
    AVSCommon
    acsdkNotifier
    acsdkAssetsInterfaces
    acsdkAssetManagerClientInterfaces
    acsdkCommunicationInterfaces
    )

# install target
asdk_install()
