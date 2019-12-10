set(SampleApp_SOURCES)
list(APPEND SampleApp_SOURCES
    CaptionPresenter.cpp
    ConnectionObserver.cpp
    ConsolePrinter.cpp
    ConsoleReader.cpp
    GuiRenderer.cpp
    InteractionManager.cpp
    KeywordObserver.cpp
    LocaleAssetsManager.cpp
    UIManager.cpp
    UserInputManager.cpp
    SampleApplication.cpp
    SampleEqualizerModeController.cpp
    main.cpp)

if (PORTAUDIO)
    list(APPEND SampleApp_SOURCES PortAudioMicrophoneWrapper.cpp)
endif()

IF (A4B)
    list(APPEND SampleApp_SOURCES RevokeAuthorizationObserver.cpp)
ENDIF()

if (PCC)
    list(APPEND SampleApp_SOURCES PhoneCaller.cpp)
endif()

if (MCC)
    list(APPEND SampleApp_SOURCES MeetingClient.cpp)
    list(APPEND SampleApp_SOURCES CalendarClient.cpp)
endif()

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER OR ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER OR
        ENDPOINT_CONTROLLERS_RANGE_CONTROLLER OR ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    add_definitions(-DENABLE_ENDPOINT_CONTROLLERS_MENU)
endif()
if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
    list(APPEND SampleApp_SOURCES PowerControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
    list(APPEND SampleApp_SOURCES ToggleControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
    list(APPEND SampleApp_SOURCES RangeControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    list(APPEND SampleApp_SOURCES ModeControllerHandler.cpp)
endif()

IF (HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    file(GLOB_RECURSE SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ExternalMediaAdapterRegistration/*.cpp)
    foreach(myfile ${SRC_FILE})
       list(APPEND SampleApp_SOURCES ${myfile})
    endforeach(myfile)
ENDIF()

add_executable(SampleApp ${SampleApp_SOURCES})

target_include_directories(SampleApp PUBLIC
    # This is relative to project(SampleApp).
    "${SampleApp_SOURCE_DIR}/include"
    "${AudioResources_SOURCE_DIR}/include"
    "${RegistrationManager_SOURCE_DIR}/include"
    "${ESP_SOURCE_DIR}/include"
    "${PORTAUDIO_INCLUDE_DIR}")

target_link_libraries(SampleApp
    DefaultClient
    AVSGatewayManager
    CapabilitiesDelegate
    CBLAuthDelegate
    SQLiteStorage
    SynchronizeStateSender
    EqualizerImplementations
    "${PORTAUDIO_LIB_PATH}")

if (PCC)
    target_link_libraries(SampleApp AVSCommon)
endif()

if (MCC)
    target_link_libraries(SampleApp AVSCommon)
endif()

if (ANDROID)
    target_include_directories(SampleApp PUBLIC "${AndroidUtilities}/include")
    target_link_libraries(SampleApp AndroidUtilities)
    if (ANDROID_MEDIA_PLAYER)
        target_include_directories(SampleApp PUBLIC "${AndroidSLESMediaPlayer}/include")
        target_link_libraries(SampleApp AndroidSLESMediaPlayer)
    endif()
endif()

if (GSTREAMER_MEDIA_PLAYER)
    target_include_directories(SampleApp PUBLIC "${MediaPlayer_SOURCE_DIR}/include")
    target_link_libraries(SampleApp MediaPlayer)
endif()

if(KWD)
    target_link_libraries(SampleApp KeywordDetectorProvider)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(SampleApp
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreServices"
        "-framework Carbon")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(SampleApp
      rt m pthread asound atomic)
endif()

