add_definitions("-DACSDK_LOG_MODULE=audioEncoder")

add_library(acsdkAudioEncoder AudioEncoder.cpp AudioEncoderFactory.cpp AudioEncoderParams.cpp AudioEncoderState.cpp)

target_include_directories(acsdkAudioEncoder PUBLIC "${acsdkAudioEncoder_SOURCE_DIR}/include")
target_include_directories(acsdkAudioEncoder PRIVATE "${acsdkAudioEncoder_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkAudioEncoder PUBLIC AVSCommon acsdkAudioEncoderInterfaces)

# install target
asdk_install()
