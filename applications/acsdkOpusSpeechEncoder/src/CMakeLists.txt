add_definitions("-DACSDK_LOG_MODULE=acsdkOpusSpeechEncoder")
add_library(acsdkOpusSpeechEncoder SpeechEncoderComponent.cpp)
target_link_libraries(acsdkOpusSpeechEncoder PUBLIC
        acsdkAudioEncoderComponent
        acsdkAudioEncoder
        acsdkOpusAudioEncoder)

# install target
asdk_install_targets(acsdkOpusSpeechEncoder FALSE)
