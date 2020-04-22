add_subdirectory("Authorization")

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(VALID FALSE)

if (PORTAUDIO AND GSTREAMER_MEDIA_PLAYER)
    set(VALID TRUE)
    add_definitions("-DPORTAUDIO")
    add_definitions("-DGSTREAMER_MEDIA_PLAYER")
elseif (ANDROID_MEDIA_PLAYER AND ANDROID_MICROPHONE)
    set(VALID TRUE)
elseif (AUDIO_INJECTION AND (GSTREAMER_MEDIA_PLAYER OR ANDROID_MEDIA_PLAYER))
    set(VALID TRUE)

    if (GSTREAMER_MEDIA_PLAYER)
        add_definitions("-DGSTREAMER_MEDIA_PLAYER")
    endif()
endif()

if (VALID)
    project(SampleApp LANGUAGES CXX)
    include(../build/BuildDefaults.cmake)

    add_subdirectory("src")
else()
    message("To build the sample app, please enable microphone and media player modules.")
endif()