cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=defaultClient")
add_library(DefaultClient
    ConnectionRetryTrigger.cpp
    DefaultClient.cpp
    DefaultClientBuilder.cpp
    DefaultClientComponent.cpp
    EqualizerRuntimeSetup.cpp
    StubApplicationAudioPipelineFactory.cpp
    )

target_include_directories(DefaultClient PUBLIC
    "${DefaultClient_SOURCE_DIR}/include"
    "${DeviceSettings_SOURCE_DIR}/include"
    "${Endpoints_SOURCE_DIR}/include")

if(BLUETOOTH_BLUEZ)
    target_link_libraries(DefaultClient BluetoothImplementationsBlueZ)
endif()

target_link_libraries(DefaultClient
    ${ACSDKDEVICESETTINGSMANAGER_LIB}
    ${ACSDKSPEECHENCODER_LIB}
    ACL
    acsdkApplicationAudioPipelineFactoryInterfaces
    acsdkCore
    acsdkManufactory
    acsdkShared
    ADSL
    AFML
    AIP
    Alexa
    ApiGateway
    AudioResources
    AVSCommon
    AVSSystem
    Captions
    CaptionsComponent
    ContextManager
    DeviceSettings
    acsdkDoNotDisturb
    Endpoints
    InterruptModel
    acsdkNotifications
    PlaybackController
    RegistrationManager
    SDKComponent
    SoftwareComponentReporter
    SpeechSynthesizer
    SystemSoundPlayer
    acsdkAlerts
    acsdkAlertsInterfaces
    acsdkAudioEncoderInterfaces
    acsdkAudioPlayer
    acsdkBluetooth
    acsdkBluetoothInterfaces
    acsdkEqualizer
    acsdkExternalMediaPlayer
    acsdkInteractionModel
    acsdkSDKClient
    acsdkStartupManagerInterfaces
    acsdkSystemClockMonitorInterfaces
    acsdkDeviceSetup
    acsdkDeviceSetupInterfaces
    acsdkAuthorization
    acsdkAuthorizationInterfaces
    acsdkSpeakerManagerComponent
    acsdkTemplateRuntime
    acsdkTemplateRuntimeInterfaces)

if (CAPTIONS)
    target_link_libraries(DefaultClient CaptionsLib)
endif()

if (PCC)
    target_link_libraries(DefaultClient acsdkPhoneCallController)
endif()

if (MC)
    target_link_libraries(DefaultClient
        acsdkMessenger
        acsdkMessagingController
    )
endif()

if (MCC)
    target_link_libraries(DefaultClient acsdkMeetingClientController)
endif()

if (METRICS)
    target_link_libraries(DefaultClient SampleMetricSink)
    target_link_libraries(DefaultClient MetricRecorder)
endif()

# install target
asdk_install()
