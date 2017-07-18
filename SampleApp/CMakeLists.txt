cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(SampleApp LANGUAGES CXX)

include(../build/BuildDefaults.cmake)

if(PORTAUDIO AND GSTREAMER_MEDIA_PLAYER)
add_subdirectory("src")
endif()
