cmake_minimum_required(VERSION 3.1)

project(acsdkApplicationAudioPipelineFactoryInterfaces LANGUAGES CXX)

add_library(acsdkApplicationAudioPipelineFactoryInterfaces INTERFACE)

target_include_directories(acsdkApplicationAudioPipelineFactoryInterfaces INTERFACE
        "${acsdkApplicationAudioPipelineFactoryInterfaces_SOURCE_DIR}/include"
        )

target_link_libraries(acsdkApplicationAudioPipelineFactoryInterfaces INTERFACE
        AVSCommon)

add_subdirectory("test")

# install interface
asdk_install_interface()
