add_definitions("-DACSDK_LOG_MODULE=settings")


add_library(DeviceSettings SHARED
        Storage/SQLiteDeviceSettingStorage.cpp)

set_target_properties(DeviceSettings PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(DeviceSettings PUBLIC
    "${AVSCommon_SOURCE_DIR}/include"
    "${CertifiedSender_SOURCE_DIR}/include"
    "${DeviceSettings_SOURCE_DIR}/include"
    "${SQLiteStorage_SOURCE_DIR}/include")

target_link_libraries(DeviceSettings AVSCommon CertifiedSender SQLiteStorage)

# install target
asdk_install()
