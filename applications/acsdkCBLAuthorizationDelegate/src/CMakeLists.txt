add_definitions("-DACSDK_LOG_MODULE=acsdkCBLAuthorizationDelegate")
add_library(acsdkCBLAuthorizationDelegate
    AuthorizationDelegateComponent.cpp)

target_include_directories(acsdkCBLAuthorizationDelegate PUBLIC
    "${acsdkCBLAuthorizationDelegate_SOURCE_DIR}/include")

target_link_libraries(acsdkCBLAuthorizationDelegate
    acsdkManufactory
    AVSCommon
    CBLAuthDelegate)

# install target
asdk_install()
