add_definitions("-DACSDK_LOG_MODULE=alerts")

add_library(Alerts SHARED
        Renderer/Renderer.cpp
        Storage/SQLiteAlertStorage.cpp
        Alarm.cpp
        Alert.cpp
        AlertsCapabilityAgent.cpp
        AlertScheduler.cpp
        Reminder.cpp
        Timer.cpp)

target_include_directories(Alerts PUBLIC
        "${Alerts_SOURCE_DIR}/include"
        "${AudioResources_SOURCE_DIR}/include"
        "${CertifiedSender_SOURCE_DIR}/include"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "${RegistrationManager_SOURCE_DIR}/include")

target_link_libraries(Alerts AudioResources AVSCommon CertifiedSender SQLiteStorage RegistrationManager)

# install target
asdk_install()
