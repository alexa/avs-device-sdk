add_definitions("-DACSDK_LOG_MODULE=acsdkShutdownManager")
add_library(acsdkShutdownManager
    ShutdownManager.cpp
    ShutdownNotifier.cpp)

target_include_directories(acsdkShutdownManager PUBLIC
    "${acsdkShutdownManager_SOURCE_DIR}/include")

target_link_libraries(acsdkShutdownManager PUBLIC
    acsdkNotifier
    acsdkShutdownManagerInterfaces
    AVSCommon)

# install target
asdk_install()
