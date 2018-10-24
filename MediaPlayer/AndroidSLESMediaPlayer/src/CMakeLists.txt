add_definitions("-DACSDK_LOG_MODULE=androidSLESMediaPlayer")
add_library(AndroidSLESMediaPlayer SHARED
        AndroidSLESMediaQueue.cpp
        AndroidSLESMediaPlayer.cpp
        AndroidSLESSpeaker.cpp
        FFmpegAttachmentInputController.cpp
        FFmpegDecoder.cpp
        FFmpegDeleter.cpp
        FFmpegStreamInputController.cpp
        FFmpegUrlInputController.cpp
        PlaybackConfiguration.cpp)

target_include_directories(AndroidSLESMediaPlayer PUBLIC
        ${AndroidSLESMediaPlayer_SOURCE_DIR}/include
        ${AndroidUtilities_SOURCE_DIR}/include
        ${PlaylistParser_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIR})

target_link_libraries(AndroidSLESMediaPlayer
        AndroidUtilities
        AVSCommon
        PlaylistParser
        OpenSLES
        EqualizerImplementations
        # FFmpeg libraries
        avcodec
        avutil
        avformat
        swresample)

# install target
asdk_install()
