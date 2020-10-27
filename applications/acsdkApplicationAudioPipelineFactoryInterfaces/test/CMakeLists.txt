cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(ApplicationAudioPipelineFactoryTestLib INTERFACE)

    target_include_directories(ApplicationAudioPipelineFactoryTestLib INTERFACE "${acsdkApplicationAudioPipelineFactoryInterfaces_SOURCE_DIR}/test")

    target_link_libraries(ApplicationAudioPipelineFactoryTestLib INTERFACE acsdkApplicationAudioPipelineFactoryInterfaces SDKInterfacesTests gmock_main)

endif()
