cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=defaultClient")
add_library(DefaultClient SHARED
    DefaultClient.cpp
    EqualizerRuntimeSetup.cpp
    )
target_include_directories(DefaultClient PUBLIC
    "${DefaultClient_SOURCE_DIR}/include"
    "${MRM_SOURCE_DIR}/include"
    "${DeviceSettings_SOURCE_DIR}/include")

if(BLUETOOTH_BLUEZ)
    target_link_libraries(DefaultClient
        BluetoothImplementationsBlueZ)
endif()

target_link_libraries(DefaultClient
    ACL
    ADSL
    AFML
    AIP
    Alerts
    AudioPlayer
    AVSCommon
    AVSSystem
    Bluetooth
    ContextManager
    DoNotDisturbCA
    Equalizer
    ExternalMediaPlayer
    InteractionModel
    Notifications
    PlaybackController
    RegistrationManager
    SpeakerManager
    SpeechSynthesizer
    Settings
    DeviceSettings
    TemplateRuntime
    )

if (COMMS)
    target_link_libraries(DefaultClient CallManager)
endif()

if (PCC)
    target_link_libraries(DefaultClient PhoneCallController)
endif()

if (MRM)
    target_link_libraries(DefaultClient MRM MRMHandler)
endif()

if (OPUS)
    target_link_libraries(DefaultClient OpusEncoderContext)
endif()

# install target
asdk_install()
