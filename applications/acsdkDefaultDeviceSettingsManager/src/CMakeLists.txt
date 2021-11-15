add_definitions("-DACSDK_LOG_MODULE=acsdkDefaultDeviceSettingsManager")
add_library(acsdkDefaultDeviceSettingsManager
        DeviceSettingsManagerBuilder.cpp
        DeviceSettingsManagerComponent.cpp)

target_include_directories(acsdkDefaultDeviceSettingsManager PUBLIC
        "${acsdkDefaultDeviceSettingsManager_SOURCE_DIR}/include")

target_link_libraries(acsdkDefaultDeviceSettingsManager
        AIP
        AVSCommon
        AVSSystem
        acsdkAlerts
        DeviceSettings
        acsdkDoNotDisturb
        acsdkManufactory
        RegistrationManager)

# install target
asdk_install()
