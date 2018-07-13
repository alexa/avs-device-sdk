add_definitions("-DACSDK_LOG_MODULE=mediaPlayer")
add_library(MediaPlayer SHARED
    AttachmentReaderSource.cpp
    BaseStreamSource.cpp
    ErrorTypeConversion.cpp
    IStreamSource.cpp
    MediaPlayer.cpp
    Normalizer.cpp
    OffsetManager.cpp)

target_include_directories(MediaPlayer PUBLIC
    "${MediaPlayer_SOURCE_DIR}/include"
    "${GST_INCLUDE_DIRS}"
    "${PlaylistParser_SOURCE_DIR}/include")

target_link_libraries(MediaPlayer "${GST_LDFLAGS}" AVSCommon PlaylistParser)

# install target
asdk_install()
