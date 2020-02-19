add_definitions("-DACSDK_LOG_MODULE=avsGatewayManager")
add_library(AVSGatewayManager SHARED
    AVSGatewayManager.cpp
    Storage/AVSGatewayManagerStorage.cpp
    PostConnectVerifyGatewaySender.cpp)

target_include_directories(AVSGatewayManager PUBLIC
        "$<BUILD_INTERFACE:${AVSGatewayManager_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/SDKInterfaces/include>"
        "$<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/Utils/include>"
        "$<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/AVS/include>"
        "$<BUILD_INTERFACE:${RegistrationManager_SOURCE_DIR}/AVS/include>"
        "$<INSTALL_INTERFACE:include>")

target_link_libraries(AVSGatewayManager AVSCommon RegistrationManager)

# install target
asdk_install()
