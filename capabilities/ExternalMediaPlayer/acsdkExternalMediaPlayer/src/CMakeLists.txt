add_definitions("-DACSDK_LOG_MODULE=ExternalMediaPlayer")

add_library(acsdkExternalMediaPlayer ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    AuthorizedSender.cpp
    ExternalMediaAdapterHandler.cpp
    ExternalMediaPlayer.cpp
    ExternalMediaPlayerComponent.cpp
    StaticExternalMediaPlayerAdapterHandler.cpp)

target_include_directories(acsdkExternalMediaPlayer PUBLIC
    "${acsdkExternalMediaPlayer_SOURCE_DIR}/include")

target_link_libraries(acsdkExternalMediaPlayer
    acsdkApplicationAudioPipelineFactoryInterfaces
    acsdkExternalMediaPlayerInterfaces
    AVSCommon
    CertifiedSender
    acsdkManufactory
    acsdkShutdownManagerInterfaces
    acsdkStartupManagerInterfaces)

# install target
asdk_install()
