add_definitions("-DACSDK_LOG_MODULE=speechSynthesizer")

add_library(SpeechSynthesizer
        SpeechSynthesizer.cpp)

target_include_directories(SpeechSynthesizer PUBLIC
        "${ContextManager_INCLUDE_DIRS}"
        "${Captions_INCLUDE_DIRS}"
        "${SpeechSynthesizer_SOURCE_DIR}/include")

target_link_libraries(SpeechSynthesizer AVSCommon Captions acsdkApplicationAudioPipelineFactoryInterfaces)

# install target
asdk_install()
