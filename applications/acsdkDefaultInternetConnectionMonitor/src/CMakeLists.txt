add_definitions("-DACSDK_LOG_MODULE=acsdkDefaultInternetConnectionMonitor")
add_library(acsdkDefaultInternetConnectionMonitor ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    InternetConnectionMonitorComponent.cpp)

target_include_directories(acsdkDefaultInternetConnectionMonitor PUBLIC
    "${acsdkDefaultInternetConnectionMonitor_SOURCE_DIR}/include")

target_link_libraries(acsdkDefaultInternetConnectionMonitor
    acsdkShared
    AVSCommon)

# install target
asdk_install()
