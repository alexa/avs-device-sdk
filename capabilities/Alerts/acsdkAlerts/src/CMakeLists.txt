add_definitions("-DACSDK_LOG_MODULE=alerts")

add_library(acsdkAlerts
        Renderer/Renderer.cpp
        Storage/SQLiteAlertStorage.cpp
        Alarm.cpp
        Alert.cpp
        AlertsCapabilityAgent.cpp
        AlertsComponent.cpp
        AlertScheduler.cpp
        Reminder.cpp
        Timer.cpp)

target_include_directories(acsdkAlerts PUBLIC
        "${acsdkAlerts_SOURCE_DIR}/include")

target_link_libraries(acsdkAlerts PUBLIC
        acsdkAlertsInterfaces
        acsdkSystemClockMonitorInterfaces
        DeviceSettings)

target_link_libraries(acsdkAlerts PRIVATE
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkManufactory
        AudioResources
        AVSCommon
        CertifiedSender
        SQLiteStorage
        RegistrationManager
        acsdkShutdownManagerInterfaces)

# install target
asdk_install()
