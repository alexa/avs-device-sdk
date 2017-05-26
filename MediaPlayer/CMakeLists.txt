#
# Setup the MediaPlayer build.
#
# To build the GStreamer based MediaPlayer, run the following command,
#     cmake <path-to-source> -DGSTREAMER_MEDIA_PLAYER=ON.
#
#
option(GSTREAMER_MEDIA_PLAYER "Enable GStreamer based media player." OFF)

if(NOT GSTREAMER_MEDIA_PLAYER)
    message("Skip building the GStreamer based media player.")
    return()
endif()

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(MediaPlayer LANGUAGES CXX)

find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.8 gstreamer-app-1.0>=1.8)

if(NOT PKG_CONFIG_FOUND)
    message("GStreamer not found. Skip building the GStreamer based media player.")
    return()
endif()

add_definitions(-DGSTREAMER_MEDIA_PLAYER)

include(../build/BuildDefaults.cmake)

add_subdirectory("src")
add_subdirectory("test")
