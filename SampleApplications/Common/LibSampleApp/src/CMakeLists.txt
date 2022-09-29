set(acsdkSampleApp_SOURCES)
list(APPEND acsdkSampleApp_SOURCES
        CaptionPresenter.cpp
        ConsoleReader.cpp
        ExternalCapabilitiesBuilder.cpp
        GuiRenderer.cpp
        LocaleAssetsManager.cpp
        UserInputManager.cpp
        SampleApplicationComponent.cpp
        SampleApplication.cpp
        SampleEqualizerModeController.cpp)

if (DIAGNOSTICS)
    list(APPEND acsdkSampleApp_SOURCES SDKDiagnostics.cpp)
endif()

IF (A4B)
    list(APPEND acsdkSampleApp_SOURCES RevokeAuthorizationObserver.cpp)
ENDIF()

if(EXTERNAL_MEDIA_ADAPTERS)
    list(APPEND acsdkSampleApp_SOURCES ${ALL_EMP_ADAPTER_REGISTRATION_FILES})
endif ()

add_library(acsdkSampleApp ${acsdkSampleApp_SOURCES})

target_include_directories(acsdkSampleApp PUBLIC
        # This is relative to project(SampleApp).
        "${acsdkSampleApp_SOURCE_DIR}/include")

target_link_libraries(acsdkSampleApp
        acsdkAudioInputStream
        acsdkCore
        acsdkCrypto
        acsdkEqualizerImplementations
        acsdkKWDImplementations
        acsdkManufactory
        acsdkSampleApplicationCBLAuthRequester
        acsdkSampleConsole
        acsdkSampleEndpoint
        acsdkSampleInteractionManager
        acsdkSamplePeripheralEndpoint
        AVSCommon
        AVSGatewayManager
        CapabilitiesDelegate
        CaptionsComponent
        DefaultClient
        InterruptModel
        SQLiteStorage
        SynchronizeStateSender)

if (ACS_UTILS)
    target_link_libraries(acsdkSampleApp acsdkACSSampleApplicationOptions)
else()
    target_link_libraries(acsdkSampleApp acsdkDefaultSampleApplicationOptions)
endif()

if(ASSET_MANAGER)
    target_link_libraries(acsdkSampleApp acsdkAssetManager acsdkAssetManagerClient)
endif()

if (ANDROID)
    target_include_directories(acsdkSampleApp PUBLIC "${AndroidUtilities}/include")
    target_link_libraries(acsdkSampleApp AndroidUtilities)
    if (ANDROID_MEDIA_PLAYER)
        target_include_directories(acsdkSampleApp PUBLIC "${AndroidSLESMediaPlayer}/include")
        target_link_libraries(acsdkSampleApp AndroidSLESMediaPlayer)
    endif()
endif()

if (GSTREAMER_MEDIA_PLAYER)
    target_include_directories(acsdkSampleApp PUBLIC "${MediaPlayer_SOURCE_DIR}/include")
    target_link_libraries(acsdkSampleApp MediaPlayer)
elseif (CUSTOM_MEDIA_PLAYER)
    target_link_libraries(acsdkSampleApp ${CUSTOM_MEDIA_PLAYER_TARGETS})
endif()

if(KWD)
    target_link_libraries(acsdkSampleApp KeywordDetectorProvider)
endif()

if (COMMS)
    target_link_libraries(acsdkSampleApp CallManager)
endif()

if (RTCSC)
    target_link_libraries(acsdkSampleApp RtcscCapabilityAgent)
endif()

if (MRM AND MRM_STANDALONE_APP)
    target_link_libraries(acsdkSampleApp acsdkMultiRoomMusic MRMHandlerProxy)
elseif (MRM)
    target_link_libraries(acsdkSampleApp acsdkMultiRoomMusic MRMHandler)
endif()

if (DIAGNOSTICS)
    target_link_libraries(acsdkSampleApp Diagnostics)
endif()

if(EXTERNAL_MEDIA_ADAPTERS)
    target_link_libraries(acsdkSampleApp acsdkExternalMediaPlayerAdapters)
endif ()

if (PORTAUDIO)
    target_link_libraries(acsdkSampleApp acsdkPortAudioMicrophoneAdapter)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(acsdkSampleApp
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework CoreServices"
            "-framework Carbon")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(acsdkSampleApp
            rt m pthread asound atomic)
endif()

target_link_libraries(acsdkSampleApp acsdkAuthorization)

if (PKCS11)
    target_link_libraries(acsdkSampleApp acsdkPkcs11)
    target_compile_definitions(acsdkSampleApp PRIVATE ENABLE_PKCS11)
endif()


if (PCC)
    target_link_libraries(acsdkSampleApp acsdkPhoneCallControllerAdapter)
endif()

if (MCC)
    target_link_libraries(acsdkSampleApp acsdkMeetingClientControllerAdapter)
endif()

add_rpath_to_target("acsdkSampleApp")

# install target only if sample libraries are installed
asdk_sample_install()
