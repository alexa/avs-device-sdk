add_definitions("-DACSDK_LOG_MODULE=diagnostics")

add_library(Diagnostics
        DevicePropertyAggregator.cpp
        DiagnosticsUtils.cpp
        DeviceProtocolTracer.cpp
        FileBasedAudioInjector.cpp
        AudioInjectorMicrophone.cpp)

target_include_directories(Diagnostics PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        "${CapabilityAgents_SOURCE_DIR}/include"
        "${Diagnostics_SOURCE_DIR}/include"
        )

target_link_libraries(Diagnostics
        acsdkAlertsInterfaces
        acsdkAudioPlayerInterfaces
        acsdkNotificationsInterfaces
        acsdkEqualizerInterfaces
        AudioResources
        AVSCommon
        DeviceSettings
        )

# install target
asdk_install()
