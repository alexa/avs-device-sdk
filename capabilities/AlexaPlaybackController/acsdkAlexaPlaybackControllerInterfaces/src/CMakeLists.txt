add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaPlaybackControllerInterfaces")

add_library(acsdkAlexaPlaybackControllerInterfaces
        PlaybackOperation.cpp
        PlaybackState.cpp)

target_include_directories(acsdkAlexaPlaybackControllerInterfaces PUBLIC
        "${acsdkAlexaPlaybackControllerInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaPlaybackControllerInterfaces AVSCommon Alexa)

# install target
asdk_install()
