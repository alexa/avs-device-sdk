add_definitions("-DACSDK_LOG_MODULE=acsdkPostConnectOperationProviderRegistrar")

add_library(acsdkPostConnectOperationProviderRegistrar
    PostConnectOperationProviderRegistrar.cpp)

target_include_directories(acsdkPostConnectOperationProviderRegistrar PUBLIC
    "${acsdkPostConnectOperationProviderRegistrar_SOURCE_DIR}/include")

target_link_libraries(acsdkPostConnectOperationProviderRegistrar
    acsdkPostConnectOperationProviderRegistrarInterfaces
    acsdkStartupManager
    AVSCommon)

# install target
asdk_install()
