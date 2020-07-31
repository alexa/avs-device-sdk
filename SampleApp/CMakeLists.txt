add_subdirectory("Authorization")

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
    if (PORTAUDIO)
      add_definitions("-DPORTAUDIO")
    endif()

    if (GSTREAMER_MEDIA_PLAYER)
      add_definitions("-DGSTREAMER_MEDIA_PLAYER")
    elseif (CUSTOM_MEDIA_PLAYER)
      add_definitions("-DCUSTOM_MEDIA_PLAYER")
    endif()

    project(SampleApp LANGUAGES CXX)
    include(../build/BuildDefaults.cmake)

    add_subdirectory("src")
else()
    message("To build the sample app, please enable microphone and media player modules.")
endif()