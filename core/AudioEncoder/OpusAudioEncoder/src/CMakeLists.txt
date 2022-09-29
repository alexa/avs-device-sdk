add_definitions("-DACSDK_LOG_MODULE=opusAudioEncoder")

add_library(acsdkOpusAudioEncoder AudioEncoderFactory.cpp OpusAudioEncoder.cpp)

target_include_directories(acsdkOpusAudioEncoder
    PUBLIC "${acsdkOpusAudioEncoder_SOURCE_DIR}/include"
    PRIVATE ${OPUS_INCLUDE_DIR} "${acsdkOpusAudioEncoder_SOURCE_DIR}/privateInclude"
)

target_link_libraries(acsdkOpusAudioEncoder
    PUBLIC
        acsdkAudioEncoderInterfaces
        AVSCommon
    PRIVATE
        ${OPUS_LIBRARY}
)

# install target
asdk_install()
