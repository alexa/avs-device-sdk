add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaRemoteVideoPlayer")

add_library(acsdkAlexaRemoteVideoPlayer
        "AlexaRemoteVideoPlayerCapabilityAgent.cpp"
        "AlexaRemoteVideoPlayerFactory.cpp")

target_include_directories(acsdkAlexaRemoteVideoPlayer PRIVATE
        "${acsdkAlexaRemoteVideoPlayer_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaRemoteVideoPlayer PUBLIC
        "${acsdkAlexaRemoteVideoPlayer_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaRemoteVideoPlayer acsdkAlexaRemoteVideoPlayerInterfaces AVSCommon AlexaVideoCommon)

# install target
asdk_install()