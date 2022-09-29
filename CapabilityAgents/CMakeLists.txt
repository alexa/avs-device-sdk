cmake_minimum_required(VERSION 3.0)
project(CapabilityAgents LANGUAGES CXX)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

set(CAPABILITY_AGENTS
        "AIP"
        "Alexa"
        "ApiGateway"
        "Equalizer"
        "InteractionModel"
        "Notifications"
        "PlaybackController"
        "SoftwareComponentReporter"
        "SpeakerManager"
        "SpeechSynthesizer"
        "System")

if (COMMS)
    list(APPEND CAPABILITY_AGENTS "CallManager")
endif()

if (PCC)
    list(APPEND CAPABILITY_AGENTS "PhoneCallController")
endif()

if (MCC)
    list(APPEND CAPABILITY_AGENTS "MeetingClientController")
endif()

if (RTCSC)
    list(APPEND CAPABILITY_AGENTS "RTCSessionController")
endif()

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
    list(APPEND CAPABILITY_AGENTS "PowerController")
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
    list(APPEND CAPABILITY_AGENTS "ToggleController")
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
    list(APPEND CAPABILITY_AGENTS "RangeController")
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    list(APPEND CAPABILITY_AGENTS "ModeController")
endif()

foreach (CA IN LISTS CAPABILITY_AGENTS)
    if (IS_DIRECTORY "${CapabilityAgents_SOURCE_DIR}/${CA}" AND EXISTS "${CapabilityAgents_SOURCE_DIR}/${CA}/CMakeLists.txt")
        message("Adding CapabilityAgent ${CA}")
        add_subdirectory(${CA})
    endif()
endforeach()
