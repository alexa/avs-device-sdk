add_definitions("-DACSDK_LOG_MODULE=synchronizeStateSender")
add_library(SynchronizeStateSender ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
        SynchronizeStateSenderFactory.cpp
        PostConnectSynchronizeStateSender.cpp)

target_include_directories(SynchronizeStateSender PUBLIC
        "${AVSCommon_SOURCE_DIR}"
        "${SynchronizeStateSender_SOURCE_DIR}/include")

target_link_libraries(SynchronizeStateSender
    acsdkPostConnectOperationProviderRegistrarInterfaces
    AVSCommon)

# install target
asdk_install()
