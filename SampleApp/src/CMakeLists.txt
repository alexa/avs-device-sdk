set(LibSampleApp_SOURCES)
list(APPEND LibSampleApp_SOURCES
        CaptionPresenter.cpp
        ConsolePrinter.cpp
        ConsoleReader.cpp
        ExternalCapabilitiesBuilder.cpp
        GuiRenderer.cpp
        InteractionManager.cpp
        KeywordObserver.cpp
        LocaleAssetsManager.cpp
        UserInputManager.cpp
        UIManager.cpp
        SampleApplicationComponent.cpp
        SampleApplication.cpp
        SampleEqualizerModeController.cpp)

if (DIAGNOSTICS)
    list(APPEND LibSampleApp_SOURCES SDKDiagnostics.cpp)
endif()

if (PORTAUDIO)
    list(APPEND LibSampleApp_SOURCES PortAudioMicrophoneWrapper.cpp)
endif()

IF (A4B)
    list(APPEND LibSampleApp_SOURCES RevokeAuthorizationObserver.cpp)
ENDIF()

if (PCC)
    list(APPEND LibSampleApp_SOURCES PhoneCaller.cpp)
endif()

if (MCC)
    list(APPEND LibSampleApp_SOURCES MeetingClient.cpp)
    list(APPEND LibSampleApp_SOURCES CalendarClient.cpp)
endif()

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES PeripheralEndpoint/PeripheralEndpointPowerControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES PeripheralEndpoint/PeripheralEndpointToggleControllerHandler.cpp)
    list(APPEND LibSampleApp_SOURCES DefaultEndpoint/DefaultEndpointToggleControllerHandler.cpp)

endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES PeripheralEndpoint/PeripheralEndpointRangeControllerHandler.cpp)
    list(APPEND LibSampleApp_SOURCES DefaultEndpoint/DefaultEndpointRangeControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES PeripheralEndpoint/PeripheralEndpointModeControllerHandler.cpp)
    list(APPEND LibSampleApp_SOURCES DefaultEndpoint/DefaultEndpointModeControllerHandler.cpp)
endif()

if (INPUT_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES InputControllerHandler.cpp)
endif()

if(EXTERNAL_MEDIA_ADAPTERS)
    list(APPEND LibSampleApp_SOURCES ${ALL_EMP_ADAPTER_REGISTRATION_FILES})
endif ()

add_library(LibSampleApp ${LibSampleApp_SOURCES})

target_include_directories(LibSampleApp PUBLIC
        # This is relative to project(SampleApp).
        "${SampleApp_SOURCE_DIR}/include"
        "${PORTAUDIO_INCLUDE_DIR}")

target_link_libraries(LibSampleApp
        acsdkAudioInputStream
        acsdkCore
        acsdkCrypto
        acsdkEqualizerImplementations
        acsdkKWDImplementations
        acsdkManufactory
        acsdkPkcs11
        AVSCommon
        AVSGatewayManager
        CapabilitiesDelegate
        CaptionsComponent
        CBLAuthDelegate
        DefaultClient
        InterruptModel
        SQLiteStorage
        SynchronizeStateSender
        "${PORTAUDIO_LIB_PATH}")

if (ACS_UTILS)
    target_link_libraries(LibSampleApp acsdkACSSampleApplicationOptions)
else()
    target_link_libraries(LibSampleApp acsdkDefaultSampleApplicationOptions)
endif()

if (ANDROID)
    target_include_directories(LibSampleApp PUBLIC "${AndroidUtilities}/include")
    target_link_libraries(LibSampleApp AndroidUtilities)
    if (ANDROID_MEDIA_PLAYER)
        target_include_directories(LibSampleApp PUBLIC "${AndroidSLESMediaPlayer}/include")
        target_link_libraries(LibSampleApp AndroidSLESMediaPlayer)
    endif()
endif()

if (GSTREAMER_MEDIA_PLAYER)
    target_include_directories(LibSampleApp PUBLIC "${MediaPlayer_SOURCE_DIR}/include")
    target_link_libraries(LibSampleApp MediaPlayer)
elseif (CUSTOM_MEDIA_PLAYER)
    target_link_libraries(LibSampleApp ${CUSTOM_MEDIA_PLAYER_TARGETS})
endif()

if(KWD)
    target_link_libraries(LibSampleApp KeywordDetectorProvider)
endif()

if (COMMS)
    target_link_libraries(LibSampleApp CallManager)
endif()

if (RTCSC)
    target_link_libraries(LibSampleApp RtcscCapabilityAgent)
endif()

if (MRM AND MRM_STANDALONE_APP)
    target_link_libraries(LibSampleApp acsdkMultiRoomMusic MRMHandlerProxy)
elseif (MRM)
    target_link_libraries(LibSampleApp acsdkMultiRoomMusic MRMHandler)
endif()

if (DIAGNOSTICS)
    target_link_libraries(LibSampleApp Diagnostics)
endif()

if(EXTERNAL_MEDIA_ADAPTERS)
    target_link_libraries(LibSampleApp acsdkExternalMediaPlayerAdapters)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(LibSampleApp
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework CoreServices"
            "-framework Carbon")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(LibSampleApp
            rt m pthread asound atomic)
endif()

if (AUTH_MANAGER)
    target_link_libraries(LibSampleApp acsdkAuthorization)
endif()

if (INPUT_CONTROLLER)
    target_link_libraries(LibSampleApp acsdkInputController)
endif()

if (CRYPTO_FOUND AND PKCS11)
    target_compile_definitions(LibSampleApp PRIVATE ENABLE_PKCS11)
endif()

add_rpath_to_target("LibSampleApp")

add_executable(SampleApp
        main.cpp)

target_link_libraries(SampleApp LibSampleApp)

# install target
asdk_install_targets(LibSampleApp TRUE)
