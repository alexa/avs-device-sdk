add_library(MediaPlayer SHARED
    MediaPlayer.cpp)

target_include_directories(MediaPlayer PUBLIC
    "${AVSCommon_SOURCE_DIR}/MediaPlayer/include"
    "${MediaPlayer_SOURCE_DIR}/include"
    "${GST_INCLUDE_DIRS}")
    
target_link_libraries(MediaPlayer "${GST_LIBRARIES}" AVSCommon AVSUtils)
