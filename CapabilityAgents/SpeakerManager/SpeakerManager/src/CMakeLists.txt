add_definitions("-DACSDK_LOG_MODULE=speakerManager")

add_library(acsdkSpeakerManager
        SpeakerManager.cpp
        ChannelVolumeFactory.cpp
        ChannelVolumeManager.cpp
        DefaultChannelVolumeFactory.cpp
        Factories.cpp
        SpeakerManagerMiscStorage.cpp
        SpeakerManagerConfig.cpp
        SpeakerManagerConfigHelper.cpp)

target_include_directories(acsdkSpeakerManager
        PUBLIC
        "${acsdkSpeakerManager_SOURCE_DIR}/include"
        PRIVATE
        "${acsdkSpeakerManager_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkSpeakerManager AVSCommon acsdkShutdownManagerInterfaces Endpoints)

# install target
asdk_install()
