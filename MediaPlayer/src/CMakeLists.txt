add_definitions("-DACSDK_LOG_MODULE=mediaPlayer")
add_library(MediaPlayer SHARED
    MediaPlayer.cpp)

target_include_directories(MediaPlayer PUBLIC
    "${MediaPlayer_SOURCE_DIR}/include"
    "${GST_INCLUDE_DIRS}")

target_link_libraries(MediaPlayer "${GST_LDFLAGS}" AVSCommon)
