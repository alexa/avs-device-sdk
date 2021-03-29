cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(VALID FALSE)

if (PORTAUDIO AND (GSTREAMER_MEDIA_PLAYER OR CUSTOM_MEDIA_PLAYER))
    set(VALID TRUE)
elseif (ANDROID_MEDIA_PLAYER AND ANDROID_MICROPHONE)
    set(VALID TRUE)
elseif (AUDIO_INJECTION AND (GSTREAMER_MEDIA_PLAYER OR ANDROID_MEDIA_PLAYER OR CUSTOM_MEDIA_PLAYER))
    set(VALID TRUE)
endif()

if (VALID)
    project(acsdkPreviewAlexaClient LANGUAGES CXX)
    include("${AVS_CMAKE_BUILD}/BuildDefaults.cmake")

    add_subdirectory("src")
else()
    message("To build the Preview Alexa Client, please enable microphone and media player modules.")
endif()