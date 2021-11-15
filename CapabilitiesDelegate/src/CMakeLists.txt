cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=capabilitiesDelegate")
add_library(CapabilitiesDelegate
    CapabilitiesDelegate.cpp
    DiscoveryEventSender.cpp
    PostConnectCapabilitiesPublisher.cpp
    Storage/SQLiteCapabilitiesDelegateStorage.cpp
    Utils/DiscoveryUtils.cpp)
target_include_directories(CapabilitiesDelegate PUBLIC
    ${CapabilitiesDelegate_SOURCE_DIR}/include)
target_include_directories(CapabilitiesDelegate PRIVATE
    ${RAPIDJSON_INCLUDE_DIR})
target_link_libraries(CapabilitiesDelegate
    acsdkAlexaEventProcessedNotifierInterfaces
    acsdkPostConnectOperationProviderRegistrarInterfaces
    acsdkShutdownManager
    AVSCommon
    RegistrationManager
    SQLiteStorage
    Endpoints)

# install target
asdk_install()
