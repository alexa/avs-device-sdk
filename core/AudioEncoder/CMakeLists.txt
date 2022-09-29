cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_subdirectory("AudioEncoder")
add_subdirectory("AudioEncoderComponent")
add_subdirectory("AudioEncoderInterfaces")
if(OPUS)
    add_subdirectory("OpusAudioEncoder")
endif()
