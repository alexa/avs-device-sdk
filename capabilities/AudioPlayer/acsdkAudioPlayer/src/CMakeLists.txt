add_definitions("-DACSDK_LOG_MODULE=audioplayer")
add_library(acsdkAudioPlayer SHARED
    AudioPlayer.cpp
    ProgressTimer.cpp)
target_include_directories(acsdkAudioPlayer PUBLIC
    "${acsdkAudioPlayer_SOURCE_DIR}/include")
target_link_libraries(acsdkAudioPlayer
    acsdkAudioPlayerInterfaces
    Captions
    AVSCommon)

# install target
asdk_install()
