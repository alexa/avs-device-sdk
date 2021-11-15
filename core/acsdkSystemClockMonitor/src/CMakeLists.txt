add_definitions("-DACSDK_LOG_MODULE=acsdkStartupManager")
add_library(acsdkSystemClockMonitor
        SystemClockMonitor.cpp
        SystemClockNotifier.cpp)

target_include_directories(acsdkSystemClockMonitor PUBLIC
        "${acsdkSystemClockMonitor_SOURCE_DIR}/include")

target_link_libraries(acsdkSystemClockMonitor PUBLIC
        acsdkNotifier
        acsdkSystemClockMonitorInterfaces
        AVSCommon)

# install target
asdk_install()
