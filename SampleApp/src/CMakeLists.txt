set(LibSampleApp_SOURCES)
list(APPEND LibSampleApp_SOURCES
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
    SampleEqualizerModeController.cpp)

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

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER OR ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER OR 
        ENDPOINT_CONTROLLERS_RANGE_CONTROLLER OR ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    add_definitions(-DENABLE_ENDPOINT_CONTROLLERS_MENU)
endif()
if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES PowerControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES ToggleControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES RangeControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
    list(APPEND LibSampleApp_SOURCES ModeControllerHandler.cpp)
endif()

IF (HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    file(GLOB_RECURSE SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ExternalMediaAdapterRegistration/*.cpp)
    foreach(myfile ${SRC_FILE})
       list(APPEND LibSampleApp_SOURCES ${myfile})
    endforeach(myfile)
ENDIF()

add_library(LibSampleApp SHARED ${LibSampleApp_SOURCES})

target_include_directories(LibSampleApp PUBLIC
    # This is relative to project(SampleApp).
    "${SampleApp_SOURCE_DIR}/include"
    "${AudioResources_SOURCE_DIR}/include"
    "${RegistrationManager_SOURCE_DIR}/include"
    "${ESP_SOURCE_DIR}/include"
    "${InterruptModel_SOURCE_DIR}/config"
    "${PORTAUDIO_INCLUDE_DIR}")

target_link_libraries(LibSampleApp
    DefaultClient
    AVSGatewayManager
    CapabilitiesDelegate
    CBLAuthDelegate
    SQLiteStorage
    SynchronizeStateSender
    EqualizerImplementations
    InterruptModel
    "${PORTAUDIO_LIB_PATH}")

if (PCC)
    target_link_libraries(LibSampleApp AVSCommon)
endif()

if (MCC)
    target_link_libraries(LibSampleApp AVSCommon)
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
endif()

if(KWD)
    target_link_libraries(LibSampleApp KeywordDetectorProvider)
endif()

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

add_executable(SampleApp
    main.cpp)

target_link_libraries(SampleApp LibSampleApp)

# install target
asdk_install_targets(LibSampleApp TRUE)
