add_definitions("-DACSDK_LOG_MODULE=mediaPlayer")
add_library(MediaPlayer SHARED
    AttachmentReaderSource.cpp
    BaseStreamSource.cpp
    ErrorTypeConversion.cpp
    IStreamSource.cpp
    MediaPlayer.cpp
    Normalizer.cpp
    OffsetManager.cpp)

set_target_properties(MediaPlayer PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(MediaPlayer PUBLIC
    "${MediaPlayer_SOURCE_DIR}/include"
    "${GST_INCLUDE_DIRS}"
    "${PlaylistParser_SOURCE_DIR}/include")

target_link_libraries(MediaPlayer "${GST_LDFLAGS}" AVSCommon PlaylistParser)

# install target
asdk_install()
