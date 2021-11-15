add_definitions("-DACSDK_LOG_MODULE=androidSLESMediaPlayer")
add_library(AndroidSLESMediaPlayer
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
        acsdkEqualizerImplementations
        # FFmpeg libraries
        ${FFMPEG_LIB_PATH}/libavcodec.so
        ${FFMPEG_LIB_PATH}/libavutil.so
        ${FFMPEG_LIB_PATH}/libavformat.so
        ${FFMPEG_LIB_PATH}/libavfilter.so
        ${FFMPEG_LIB_PATH}/libswresample.so
        )

# install target
asdk_install()
