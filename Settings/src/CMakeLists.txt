add_definitions("-DACSDK_LOG_MODULE=settings")


add_library(DeviceSettings SHARED
        CloudControlledSettingProtocol.cpp
        DeviceControlledSettingProtocol.cpp
        SettingEventSender.cpp
        SettingEventRequestObserver.cpp
        SettingConnectionObserver.cpp
        SharedAVSSettingProtocol.cpp
        Storage/SQLiteDeviceSettingStorage.cpp
        Types/LocaleWakeWordsSetting.cpp
        Types/NetworkInfo.cpp)

target_include_directories(DeviceSettings PUBLIC
    "${AVSCommon_SOURCE_DIR}/include"
    "${CertifiedSender_SOURCE_DIR}/include"
    "${DeviceSettings_SOURCE_DIR}/include"
    "${SQLiteStorage_SOURCE_DIR}/include")

target_link_libraries(DeviceSettings AVSCommon CertifiedSender SQLiteStorage)

# install target
asdk_install()
