if (ENABLE_DEVICE_CONTEXT_IN_EVENT)
    add_definitions("-DENABLE_DEVICE_CONTEXT_IN_EVENT")
endif()
add_definitions("-DACSDK_LOG_MODULE=audioplayer")

set (AUDIOPLAYER_SOURCES
    AudioPlayer.cpp
    AudioPlayerComponent.cpp
    ProgressTimer.cpp
    Util.cpp)

add_library(acsdkAudioPlayer ${AUDIOPLAYER_SOURCES})

target_include_directories(acsdkAudioPlayer PUBLIC
    "${acsdkAudioPlayer_SOURCE_DIR}/include"
    "${CRYPTO_INCLUDE_DIRS}")
target_link_libraries(acsdkAudioPlayer
    acsdkAudioPlayerInterfaces
    acsdkApplicationAudioPipelineFactoryInterfaces
    acsdkManufactory
    acsdkShutdownManagerInterfaces
    Captions
    AVSCommon
    acsdkCodecUtils
    acsdkCryptoInterfaces)


# install target
asdk_install()
