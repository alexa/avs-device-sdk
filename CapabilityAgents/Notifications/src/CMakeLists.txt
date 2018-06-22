add_definitions("-DACSDK_LOG_MODULE=Notifications")

add_library(Notifications SHARED
		NotificationIndicator.cpp
		NotificationRenderer.cpp
		NotificationsCapabilityAgent.cpp
        SQLiteNotificationsStorage.cpp)

target_include_directories(Notifications PUBLIC
		"${AudioResources_SOURCE_DIR}/include"
        "${Notifications_SOURCE_DIR}/include"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "${RegistrationManager_SOURCE_DIR}/include")

target_link_libraries(Notifications AudioResources AVSCommon SQLiteStorage RegistrationManager)

# install target
asdk_install()