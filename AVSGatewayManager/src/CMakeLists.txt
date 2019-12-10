add_definitions("-DACSDK_LOG_MODULE=avsGatewayManager")
add_library(AVSGatewayManager SHARED
    AVSGatewayManager.cpp
    Storage/AVSGatewayManagerStorage.cpp
    PostConnectVerifyGatewaySender.cpp)

target_include_directories(AVSGatewayManager PUBLIC
    "${AVSCommon_SOURCE_DIR}"
    "${AVSGatewayManager_SOURCE_DIR}/include")

target_link_libraries(AVSGatewayManager AVSCommon RegistrationManager)

# install target
asdk_install()
