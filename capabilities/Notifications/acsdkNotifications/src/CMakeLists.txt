add_definitions("-DACSDK_LOG_MODULE=Notifications")

add_library(acsdkNotifications
		NotificationsComponent.cpp
		NotificationIndicator.cpp
		NotificationsNotifier.cpp
        NotificationRenderer.cpp
		NotificationsCapabilityAgent.cpp
		SQLiteNotificationsStorage.cpp)

target_include_directories(acsdkNotifications PUBLIC
		"${acsdkNotifications_SOURCE_DIR}/include")

target_link_libraries(
		acsdkNotifications
		acsdkApplicationAudioPipelineFactoryInterfaces
		acsdkManufactory
		acsdkNotificationsInterfaces
		acsdkNotifier
		acsdkShutdownManagerInterfaces
		AudioResources
		AVSCommon
		SQLiteStorage
		RegistrationManager)

# install target
asdk_install()
