add_definitions("-DACSDK_LOG_MODULE=acsdkDeviceSetup")

add_library(
    acsdkDeviceSetup
    DeviceSetup.cpp
    DeviceSetupFactory.cpp
    DeviceSetupMessageRequest.cpp
    DeviceSetupComponent.cpp
)

target_include_directories(acsdkDeviceSetup PRIVATE
    "${acsdkDeviceSetup_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkDeviceSetup PUBLIC
    "${acsdkDeviceSetup_SOURCE_DIR}/include")

target_link_libraries(
    acsdkDeviceSetup
    acsdkDeviceSetupInterfaces
    acsdkManufactory
    AVSCommon
)

# install target
asdk_install()
