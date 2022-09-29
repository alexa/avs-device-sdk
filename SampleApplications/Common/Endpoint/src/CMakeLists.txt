add_definitions("-DACSDK_LOG_MODULE=acsdkSampleEndpoint")

set(SampleEndpoint_SOURCES)
list(APPEND SampleEndpoint_SOURCES
    EndpointAlexaLauncherHandler.cpp
    EndpointCapabilitiesBuilder.cpp
    EndpointFocusAdapter.cpp)

if (VIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaKeypadControllerHandler.cpp)
endif()

if (VIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaPlaybackControllerHandler.cpp)
endif()

if (VIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaSeekControllerHandler.cpp)
endif()

if (VIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaVideoRecorderHandler.cpp)
endif()

if (VIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaChannelControllerHandler.cpp)
endif()

if (VIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaRecordControllerHandler.cpp)
endif()

if (VIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER)
    list(APPEND SampleEndpoint_SOURCES EndpointAlexaRemoteVideoPlayerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_INPUT_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES EndpointInputControllerHandler.cpp)
    list(APPEND SampleEndpoint_SOURCES InputControllerEndpointCapabilitiesBuilder.cpp)
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES DefaultEndpoint/DefaultEndpointToggleControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES DefaultEndpoint/DefaultEndpointRangeControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    list(APPEND SampleEndpoint_SOURCES DefaultEndpoint/DefaultEndpointModeControllerHandler.cpp)
endif()

add_library(acsdkSampleEndpoint ${SampleEndpoint_SOURCES})

target_include_directories(acsdkSampleEndpoint PUBLIC
        "${acsdkSampleEndpoint_SOURCE_DIR}/include")

target_link_libraries(acsdkSampleEndpoint
        AVSCommon
        acsdkAlexaLauncherInterfaces
        acsdkNotifier
        acsdkPresentationOrchestratorInterfaces
        acsdkPresentationOrchestratorStateTracker
        acsdkSampleConsole
)

if (VIDEO_CONTROLLERS_ALEXA_LAUNCHER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaLauncher)
endif()

if (VIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaKeypadController
            acsdkAlexaKeypadControllerInterfaces)
endif()

if (VIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaPlaybackController
            acsdkAlexaPlaybackControllerInterfaces)
endif()

if (VIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaSeekController
            acsdkAlexaSeekControllerInterfaces)
endif()

if (VIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaVideoRecorder
            acsdkAlexaVideoRecorderInterfaces)
endif()

if (VIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaChannelController
            acsdkAlexaChannelControllerInterfaces)
endif()

if (VIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaRecordController
            acsdkAlexaRecordControllerInterfaces)
endif()

if (VIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER)
    target_link_libraries(acsdkSampleEndpoint
            acsdkAlexaRemoteVideoPlayer
            acsdkAlexaRemoteVideoPlayerInterfaces
            )
endif()

if (ENDPOINT_CONTROLLERS_INPUT_CONTROLLER)
    target_link_libraries(acsdkSampleEndpoint acsdkAlexaInputController)
endif()

# install target only if sample libraries are installed
asdk_sample_install()
