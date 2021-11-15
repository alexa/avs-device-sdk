add_definitions("-DACSDK_LOG_MODULE=acsdkOpusSpeechEncoder")
add_library(acsdkOpusSpeechEncoder
        SpeechEncoderComponent.cpp)

target_include_directories(acsdkOpusSpeechEncoder PUBLIC
        "${acsdkOpusSpeechEncoder_SOURCE_DIR}/include")

target_link_libraries(acsdkOpusSpeechEncoder
        acsdkManufactory
        OpusEncoderContext
        SpeechEncoder)

# install target
asdk_install()
