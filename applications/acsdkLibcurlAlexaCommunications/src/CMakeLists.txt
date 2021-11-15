add_definitions("-DACSDK_LOG_MODULE=acsdkLibcurlAlexaCommunications")
add_library(acsdkLibcurlAlexaCommunications
        AlexaCommunicationsComponent.cpp)

target_include_directories(acsdkLibcurlAlexaCommunications PUBLIC
    "${acsdkLibcurlAlexaCommunications_SOURCE_DIR}/include")

target_link_libraries(acsdkLibcurlAlexaCommunications
    ACL
    acsdkCore
    acsdkManufactory
    acsdkPostConnectOperationProviderRegistrarInterfaces
    acsdkShared
    acsdkShutdownManagerInterfaces
    AVSCommon)

# install target
asdk_install()
