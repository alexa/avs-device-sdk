cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=aip")
add_library(AIP
    AudioInputProcessor.cpp)
target_include_directories(AIP PUBLIC
    "${AIP_SOURCE_DIR}/include"
    "${AFML_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}"
    "${DeviceSettings_INCLUDE_DIRS}"
    "${SystemSoundPlayer_INCLUDE_DIRS}")

target_link_libraries(AIP
    AVSCommon
    ADSL
    AFML
    DeviceSettings
    acsdkAudioEncoderInterfaces
    SystemSoundPlayer
    acsdkNotifier)

# install target
asdk_install()
