add_definitions("-DACSDK_LOG_MODULE=acsdkNullSpeechEncoder")
add_library(acsdkNullSpeechEncoder
        SpeechEncoderComponent.cpp)

target_include_directories(acsdkNullSpeechEncoder PUBLIC
        "${acsdkNullSpeechEncoder_SOURCE_DIR}/include")

target_link_libraries(acsdkNullSpeechEncoder
        acsdkManufactory
        SpeechEncoder)

# install target
asdk_install()
