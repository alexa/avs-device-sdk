add_subdirectory("Authorization")

if ((PORTAUDIO AND GSTREAMER_MEDIA_PLAYER) OR (ANDROID_MEDIA_PLAYER AND ANDROID_MICROPHONE))
    cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
    project(SampleApp LANGUAGES CXX)

    include(../build/BuildDefaults.cmake)

    if (PORTAUDIO)
        add_definitions("-DPORTAUDIO")
    endif()

    if (GSTREAMER_MEDIA_PLAYER)
        add_definitions("-DGSTREAMER_MEDIA_PLAYER")
    endif()

    add_subdirectory("src")
else()
    message("To build the sample app, please enable microphone and media player modules.")
endif()
