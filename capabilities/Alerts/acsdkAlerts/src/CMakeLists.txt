add_definitions("-DACSDK_LOG_MODULE=alerts")

add_library(acsdkAlerts SHARED
        Renderer/Renderer.cpp
        Storage/SQLiteAlertStorage.cpp
        Alarm.cpp
        Alert.cpp
        AlertsCapabilityAgent.cpp
        AlertScheduler.cpp
        Reminder.cpp
        Timer.cpp)

target_include_directories(acsdkAlerts PUBLIC
        "${acsdkAlerts_SOURCE_DIR}/include")

target_link_libraries(acsdkAlerts PUBLIC
        acsdkAlertsInterfaces
        DeviceSettings)

target_link_libraries(acsdkAlerts PRIVATE
        AudioResources
        AVSCommon
        CertifiedSender
        SQLiteStorage
        RegistrationManager
        SpeakerManager)

# install target
asdk_install()
