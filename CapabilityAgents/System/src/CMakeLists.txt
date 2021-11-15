add_definitions("-DACSDK_LOG_MODULE=system")

add_library(AVSSystem
    "${CMAKE_CURRENT_LIST_DIR}/LocaleHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ReportStateHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SoftwareInfoSender.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SoftwareInfoSendRequest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/StateReportGenerator.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SystemCapabilityProvider.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SystemComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/TimeZoneHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/UserInactivityMonitor.cpp")

if (A4B)
    target_sources(AVSSystem PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/RevokeAuthorizationHandler.cpp")
endif()

target_include_directories(AVSSystem PUBLIC
        "${AVSSystem_SOURCE_DIR}/include"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "${CertifiedSender_SOURCE_DIR}/include"
        "${RegistrationManager_SOURCE_DIR}/include"
        "${DeviceSettings_SOURCE_DIR}/include")

target_link_libraries(AVSSystem
    AVSCommon
    ACL
    CertifiedSender
    SQLiteStorage
    RegistrationManager
    DeviceSettings
    acsdkManufactory
    acsdkNotifier
    acsdkShutdownManagerInterfaces)

# install target
asdk_install()
