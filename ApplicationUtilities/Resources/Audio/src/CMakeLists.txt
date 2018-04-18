cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(AudioResources SHARED
        AlertsAudioFactory.cpp
        AudioFactory.cpp
        NotificationsAudioFactory.cpp
        CommunicationsAudioFactory.cpp)

target_include_directories(AudioResources PUBLIC
        "${AudioResources_SOURCE_DIR}/include"
	"${AVSCommon_SOURCE_DIR}/Utils/include")

target_link_libraries(AudioResources AVSCommon)

# install target
asdk_install()
