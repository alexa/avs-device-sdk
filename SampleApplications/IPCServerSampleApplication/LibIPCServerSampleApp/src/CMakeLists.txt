add_definitions("-DACSDK_LOG_MODULE=LibIPCServerSampleApp")
list(APPEND LibIPCServerSampleApp_SOURCES
        AlexaPresentation/AplClientBridge.cpp
        AlexaPresentation/APLDocumentSession.cpp
        AlexaPresentation/APLDocumentSessionManager.cpp
        AlexaPresentation/APLPayloadParser.cpp
        AlexaPresentation/APLRuntimeInterfaceImpl.cpp
        AlexaPresentation/APLRuntimePresentationAdapter.cpp
        AlexaPresentation/IPCAPLAgent.cpp
        ConfigValidator.cpp
        ConnectionObserver.cpp
        CachingDownloadManager.cpp
        ConsolePrinter.cpp
        DownloadMonitor.cpp
        ExternalCapabilitiesBuilder.cpp
        GUILogBridge.cpp
        GUI/GUIActivityEventNotifier.cpp
        GUI/GUIClient.cpp
        GUI/GUIManager.cpp
        GUI/TemplateRuntimePresentationAdapterBridge.cpp
        KeywordObserver.cpp
        LocaleAssetsManager.cpp
        SampleApplication.cpp
        SampleApplicationComponent.cpp
        SampleEqualizerModeController.cpp
        SmartScreenCaptionPresenter.cpp
        SmartScreenCaptionStateManager.cpp
        TelemetrySink.cpp
        TimezoneHelper.cpp
        IPC/IPCVersionManager.cpp
        IPC/IPCHandlerBase.cpp
        IPC/IPCRouter.cpp
        IPC/Components/APLClientHandler.cpp
        IPC/Components/AlexaCaptionsHandler.cpp
        IPC/Components/AudioFocusManagerHandler.cpp
        IPC/Components/DoNotDisturbHandler.cpp
        IPC/Components/InteractionManagerHandler.cpp
        IPC/Components/LoggerHandler.cpp
        IPC/Components/SessionSetupHandler.cpp
        IPC/Components/SystemHandler.cpp
        IPC/Components/TemplateRuntimeHandler.cpp
        IPC/Components/WindowManagerHandler.cpp)

set(APLCLIENT_LIBRARY_DIRS "${APL_CLIENT_INSTALL_PATH}/lib")
set(APLCLIENT_INCLUDE_DIRS "${APLCLIENT_INCLUDE_DIRS}" "${APL_CLIENT_INSTALL_PATH}/include")
set(APLCLIENT_LDFLAGS "${APLCLIENT_LDFLAGS}" "-L${APLCLIENT_LIBRARY_DIRS}")

if (RTCSC)
    list(APPEND LibIPCServerSampleApp_SOURCES IPC/Components/LiveViewCameraHandler.cpp)
    list(APPEND LibIPCServerSampleApp_SOURCES Extensions/LiveView/AplLiveViewExtension.cpp)
    list(APPEND LibIPCServerSampleApp_SOURCES LiveViewController/LiveViewControllerPresentationAdapter.cpp)
endif ()

if (PORTAUDIO)
    list(APPEND LibIPCServerSampleApp_SOURCES PortAudioMicrophoneWrapper.cpp)
endif ()

IF (A4B)
    list(APPEND LibIPCServerSampleApp_SOURCES RevokeAuthorizationObserver.cpp)
ENDIF ()

IF (HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    file(GLOB_RECURSE SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ExternalMediaAdapterRegistration/*.cpp)
    foreach (myfile ${SRC_FILE})
        list(APPEND LibIPCServerSampleApp_SOURCES ${myfile})
    endforeach (myfile)
ENDIF ()

add_library(LibIPCServerSampleApp ${LibIPCServerSampleApp_SOURCES})

target_include_directories(LibIPCServerSampleApp PUBLIC
        "${LibIPCServerSampleApp_SOURCE_DIR}/include"
        "${ASDK_INCLUDE_DIRS}"
        "${AudioResources_SOURCE_DIR}/include"
        "${RegistrationManager_SOURCE_DIR}/include"
        "${ESP_SOURCE_DIR}/include"
        "${PORTAUDIO_INCLUDE_DIR}"
        "${Utils_SOURCE_DIR}/include"
        "${Communication_SOURCE_DIR}/include"
        "${base64_SOURCE_DIR}/include"
        "${APLCLIENT_INCLUDE_DIRS}")

target_link_libraries(LibIPCServerSampleApp
        "${ASDK_LDFLAGS}"
        "${APLCLIENT_LDFLAGS}"
        acsdkAlerts
        acsdkAlexaPresentationFeatureClient
        acsdkAudioInputStream
        acsdkAuthorization
        acsdkBluetooth
        acsdkCore
        acsdkCrypto
        acsdkEqualizerImplementations
        acsdkKWDImplementations
        acsdkManufactory
        acsdkNotifications
        acsdkNotifier
        acsdkPresentationOrchestratorFeatureClient
        acsdkSampleApplicationCBLAuthRequester
        acsdkSampleConsole
        acsdkSampleEndpoint
        acsdkSampleInteractionManager
        acsdkSamplePeripheralEndpoint
        acsdkSampleTemplateRuntimePresentationAdaptor
        acsdkSDKClient
        acsdkShared
        acsdkSpeakerManager
        acsdkVisualCharacteristicsFeatureClient
        acsdkVisualStateTrackerFeatureClient
        ACL
        APLClient
        AudioResources
        AVSCommon
        AVSGatewayManager
        CapabilitiesDelegate
        Captions
        CaptionsComponent
        CertifiedSender
        Communication
        ContextManager
        DefaultClient
        DeviceSettings
        InterruptModel
        RegistrationManager
        SQLiteStorage
        SynchronizeStateSender
        "${PORTAUDIO_LIB_PATH}")

if (RTCSC)
    target_link_libraries(LibIPCServerSampleApp acsdkLiveViewControllerFeatureClient)
endif()

if (GSTREAMER_MEDIA_PLAYER)
    target_include_directories(LibIPCServerSampleApp PUBLIC "${MediaPlayer_SOURCE_DIR}/include" "${GST_INCLUDE_DIRS}")
    target_link_libraries(LibIPCServerSampleApp MediaPlayer)
    target_include_directories(LibIPCServerSampleApp PUBLIC)
endif ()

if (KWD)
    target_link_libraries(LibIPCServerSampleApp KeywordDetectorProvider)
endif ()

if (METRICS_EXTENSION)
    add_definitions(-DMETRICS_EXTENSION)
    target_include_directories(LibIPCServerSampleApp PRIVATE "${MetricsExtension_SOURCE_DIR}/include")
    target_link_libraries(LibIPCServerSampleApp MetricsExtension)
endif()

if (NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR "WebSocketPP Path is required to enable websocket server")
endif ()

if (NOT ASIO_INCLUDE_DIR)
    find_path(ASIO_INCLUDE_DIR NAMES "asio.hpp")

    if (ASIO_INCLUDE_DIR STREQUAL "ASIO_INCLUDE_DIR-NOTFOUND")
        message(FATAL_ERROR "WebSocket support requires asio.")
    endif ()
endif ()

target_compile_definitions(LibIPCServerSampleApp PUBLIC ASIO_STANDALONE)
target_include_directories(LibIPCServerSampleApp PUBLIC
        "${ASIO_INCLUDE_DIR}"
        "${WEBSOCKETPP_INCLUDE_DIR}")

if (CRYPTO_FOUND AND PKCS11)
    target_compile_definitions(LibIPCServerSampleApp PRIVATE ENABLE_PKCS11)
endif ()

# Currently only allow non SSL websocket with debug builds
if (DISABLE_WEBSOCKET_SSL)
    string(TOLOWER ${CMAKE_BUILD_TYPE} LOWERCASE_CMAKE_BUILD_TYPE)
    if (NOT LOWERCASE_CMAKE_BUILD_TYPE STREQUAL "debug")
        message(FATAL_ERROR "DISABLE_WEBSOCKET_SSL used in non-debug build.")
    endif ()
else ()
    find_package(OpenSSL REQUIRED)
    add_definitions(-DENABLE_WEBSOCKET_SSL)

    target_include_directories(LibIPCServerSampleApp PUBLIC "${OPENSSL_INCLUDE_DIR}")
    target_link_libraries(LibIPCServerSampleApp
            "${OPENSSL_SSL_LIBRARY}"
            "${OPENSSL_CRYPTO_LIBRARY}")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(LibIPCServerSampleApp
            "-rpath ${ASDK_LIBRARY_DIRS}"
            "-rpath ${APLCLIENT_LIBRARY_DIRS}"
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework CoreServices"
            "-framework Carbon")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(LibIPCServerSampleApp
            rt m pthread asound atomic
            "-Wl,-rpath,${ASDK_LIBRARY_DIRS}"
            "-Wl,-rpath,${APLCLIENT_LIBRARY_DIRS}")
endif ()

add_rpath_to_target("LibIPCServerSampleApp")
