cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (GSTREAMER_MEDIA_PLAYER)
    add_subdirectory("GStreamerMediaPlayer")
elseif (ANDROID_MEDIA_PLAYER)
    add_subdirectory("AndroidSLESMediaPlayer")
else()
    message("No media player will be built.")
endif()


