add_definitions("-DACSDK_LOG_MODULE=AssetManager")

set(acsdkAssetManager_SOURCES
    AssetManager.cpp
    DavsRequester.cpp
    Requester.cpp
    RequesterFactory.cpp
    RequesterMetadata.cpp
    RequestFactory.cpp
    Resource.cpp
    StorageManager.cpp
    UrlAllowListWrapper.cpp
    UrlRequester.cpp
    )
set(acsdkAssetManager_INCLUDES
    ${acsdkAssetManager_SOURCE_DIR}/include
    )
set(acsdkAssetManager_LIBRARIES
    AVSCommon
    acsdkAssetManagerClient
    )

add_library(acsdkAssetManager ${acsdkAssetManager_SOURCES})
target_include_directories(acsdkAssetManager PUBLIC ${acsdkAssetManager_INCLUDES})
target_link_libraries(acsdkAssetManager ${acsdkAssetManager_LIBRARIES} acsdkDavsClient)

# install target
asdk_install()

# Setup the Testing library
add_library(acsdkAssetManagerForTesting ${acsdkAssetManager_SOURCES})
target_include_directories(acsdkAssetManagerForTesting PUBLIC ${acsdkAssetManager_INCLUDES})
target_link_libraries(acsdkAssetManagerForTesting ${acsdkAssetManager_LIBRARIES} acsdkDavsClientForTesting)
target_compile_definitions(acsdkAssetManagerForTesting PUBLIC UNIT_TEST=1)
