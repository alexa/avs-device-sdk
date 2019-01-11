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
set_target_properties(Alerts PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(Alerts PUBLIC
        "${Alerts_SOURCE_DIR}/include"
        "${AudioResources_SOURCE_DIR}/include"
        "${CertifiedSender_SOURCE_DIR}/include"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "${RegistrationManager_SOURCE_DIR}/include")

target_link_libraries(Alerts AudioResources AVSCommon CertifiedSender SQLiteStorage RegistrationManager)

# install target
asdk_install()
