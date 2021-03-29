#
# Set up FFmpeg specific configurations for the sample app.
#
# To build with FFMpeg, run the following command,
#     cmake <path-to-source>
#       -DFFMPEG_LIB_PATH=<path-to-ffmpeg-lib>
#       -DFFMPEG_INCLUDE_DIR=<path-to-ffmpeg-include-dir>
#
option(FFMPEG_LIB_PATH, "Path to FFmpeg shared libraries")
option(FFMPEG_INCLUDE_DIR, "Path to FFmpeg include directory")

if (FFMPEG_LIB_PATH AND FFMPEG_INCLUDE_DIR)
    message("Including FFmpeg libraries")
    link_directories(${FFMPEG_LIB_PATH})
endif()
