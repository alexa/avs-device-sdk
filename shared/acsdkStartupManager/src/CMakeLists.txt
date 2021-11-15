add_definitions("-DACSDK_LOG_MODULE=acsdkStartupManager")
add_library(acsdkStartupManager
    StartupManager.cpp
    StartupNotifier.cpp)

target_include_directories(acsdkStartupManager PUBLIC
    "${acsdkStartupManager_SOURCE_DIR}/include")

target_link_libraries(acsdkStartupManager PUBLIC
    acsdkNotifier
    acsdkStartupManagerInterfaces
    AVSCommon)

# install target
asdk_install()
