cmake_minimum_required(VERSION 3.0)
project(CapabilityAgents LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

add_subdirectory("AIP")
add_subdirectory("Alerts")
add_subdirectory("Alexa")
add_subdirectory("ApiGateway")
add_subdirectory("AudioPlayer")
add_subdirectory("Bluetooth")
add_subdirectory("DoNotDisturb")
add_subdirectory("Equalizer")
add_subdirectory("ExternalMediaPlayer")
add_subdirectory("InteractionModel")
add_subdirectory("MRM")
add_subdirectory("Notifications")
add_subdirectory("PlaybackController")
add_subdirectory("SpeakerManager")
add_subdirectory("SpeechSynthesizer")
add_subdirectory("System")
add_subdirectory("TemplateRuntime")

if (COMMS)
    add_subdirectory("CallManager")
endif()

if (PCC)
    add_subdirectory("PhoneCallController")
endif()

if (MCC)
    add_subdirectory("MeetingClientController")
endif()

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
    add_subdirectory("PowerController")
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
    add_subdirectory("ToggleController")
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
    add_subdirectory("RangeController")
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    add_subdirectory("ModeController")
endif()

