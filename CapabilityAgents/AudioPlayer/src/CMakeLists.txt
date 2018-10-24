add_definitions("-DACSDK_LOG_MODULE=audioplayer")
add_library(AudioPlayer SHARED
    AudioPlayer.cpp
    ProgressTimer.cpp)
target_include_directories(AudioPlayer PUBLIC
    "${AudioPlayer_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(AudioPlayer
    AVSCommon)

# install target
asdk_install()
