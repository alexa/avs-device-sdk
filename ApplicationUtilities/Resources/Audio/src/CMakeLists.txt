cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(AudioResources
        AlertsAudioFactory.cpp
        AudioFactory.cpp
        NotificationsAudioFactory.cpp
        CommunicationsAudioFactory.cpp
        SystemSoundAudioFactory.cpp)

target_include_directories(AudioResources PUBLIC
        "${AudioResources_SOURCE_DIR}/include")

target_link_libraries(AudioResources AVSCommon)

# install target
asdk_install()
