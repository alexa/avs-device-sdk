cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=acsdkGstreamerApplicationAudioPipelineFactory")

add_library(acsdkGstreamerApplicationAudioPipelineFactory
        ApplicationAudioPipelineFactoryComponent.cpp
        GstreamerApplicationAudioPipelineFactory.cpp)

target_include_directories(acsdkGstreamerApplicationAudioPipelineFactory PUBLIC
        ${acsdkGstreamerApplicationAudioPipelineFactory_SOURCE_DIR}/include)

target_link_libraries(acsdkGstreamerApplicationAudioPipelineFactory
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkEqualizerInterfaces
        acsdkManufactory
        acsdkShutdownManagerInterfaces
        AVSCommon
        Captions
        MediaPlayer)

# install target
asdk_install()
