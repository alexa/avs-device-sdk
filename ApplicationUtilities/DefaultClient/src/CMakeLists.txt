cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=defaultClient")
add_library(DefaultClient SHARED
    DefaultClient.cpp)
target_include_directories(DefaultClient PUBLIC
    "${DefaultClient_SOURCE_DIR}/include")

if(BLUETOOTH_BLUEZ)
    target_link_libraries(DefaultClient
        BluetoothImplementationsBlueZ)
endif()

target_link_libraries(DefaultClient
    AVSCommon
    ACL
    ADSL
    AFML
    AIP
    Alerts
    Bluetooth
    Notifications
    PlaybackController
    SpeakerManager
    SpeechSynthesizer
    Settings
    TemplateRuntime
    AudioPlayer
    ExternalMediaPlayer
    AVSSystem
    ContextManager
    RegistrationManager)

if (COMMS)
    target_link_libraries(DefaultClient CallManager)
endif()
# install target
asdk_install()
