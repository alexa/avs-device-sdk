cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(MediaPlayer LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

if(NOT GSTREAMER_MEDIA_PLAYER)
    message("GStreamer based media player will not be built.")
else()
    add_subdirectory("src")
    acsdk_add_test_subdirectory_if_allowed()
endif()
