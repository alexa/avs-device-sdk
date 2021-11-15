add_definitions("-DACSDK_LOG_MODULE=acsdkCore")
add_library(acsdkCore
    CoreComponent.cpp)

target_include_directories(acsdkCore PUBLIC
    "${acsdkCore_SOURCE_DIR}/include")

target_link_libraries(acsdkCore
    acsdkManufactory
    acsdkPostConnectOperationProviderRegistrar
    acsdkShared
    ADSL
    AFML
    AVSCommon
    AVSGatewayManager
    CapabilitiesDelegate
    ContextManager
    RegistrationManager
    SQLiteStorage
    SynchronizeStateSender)

# install target
asdk_install()
