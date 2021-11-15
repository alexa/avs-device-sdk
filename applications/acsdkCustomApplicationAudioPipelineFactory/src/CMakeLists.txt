cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=acsdkCustomApplicationAudioPipelineFactory")

add_library(acsdkCustomApplicationAudioPipelineFactory
		ApplicationAudioPipelineFactoryComponent.cpp
        CustomApplicationAudioPipelineFactory.cpp)

target_include_directories(acsdkCustomApplicationAudioPipelineFactory PUBLIC
        ${acsdkCustomApplicationAudioPipelineFactory_SOURCE_DIR}/include)

target_link_libraries(acsdkCustomApplicationAudioPipelineFactory
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkEqualizerInterfaces
        acsdkManufactory
        acsdkShutdownManagerInterfaces
        AVSCommon
        Captions
        ${CUSTOM_MEDIA_PLAYER_TARGETS})

# install target
asdk_install()
