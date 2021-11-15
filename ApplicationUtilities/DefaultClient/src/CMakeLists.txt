cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=defaultClient")
add_library(DefaultClient
    ConnectionRetryTrigger.cpp
    DefaultClient.cpp
    DefaultClientComponent.cpp
    EqualizerRuntimeSetup.cpp
    StubApplicationAudioPipelineFactory.cpp
    )

target_include_directories(DefaultClient PUBLIC
    "${DefaultClient_SOURCE_DIR}/include"
    "${DeviceSettings_SOURCE_DIR}/include"
    "${Endpoints_SOURCE_DIR}/include")

if(BLUETOOTH_BLUEZ)
    target_link_libraries(DefaultClient
        BluetoothImplementationsBlueZ)
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
    SpeakerManager
    SpeechSynthesizer
    SystemSoundPlayer
    TemplateRuntime
    acsdkAlerts
    acsdkAlertsInterfaces
    acsdkAudioPlayer
    acsdkBluetooth
    acsdkBluetoothInterfaces
    acsdkEqualizer
    acsdkExternalMediaPlayer
    acsdkInteractionModel
    acsdkStartupManagerInterfaces
    acsdkSystemClockMonitorInterfaces
    acsdkDeviceSetup
    acsdkDeviceSetupInterfaces
    acsdkAuthorization
    acsdkAuthorizationInterfaces)

if (CAPTIONS)
    target_link_libraries(DefaultClient CaptionsLib)
endif()

if (PCC)
    target_link_libraries(DefaultClient PhoneCallController)
endif()

if (MC)
    target_link_libraries(DefaultClient
        acsdkMessenger
        acsdkMessagingController
    )
endif()

if (MCC)
    target_link_libraries(DefaultClient MeetingClientController)
endif()

if (METRICS)
    target_link_libraries(DefaultClient SampleMetricSink)
    target_link_libraries(DefaultClient MetricRecorder)
endif()

if (OPUS)
    target_link_libraries(DefaultClient OpusEncoderContext)
endif()

# install target
asdk_install()
