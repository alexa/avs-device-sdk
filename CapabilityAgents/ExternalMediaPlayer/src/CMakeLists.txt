add_definitions("-DACSDK_LOG_MODULE=ExternalMediaPlayer")
set(ExternalMediaPlayer_SOURCES )
list(APPEND ExternalMediaPlayer_SOURCES ExternalMediaPlayer.cpp)

if(HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    add_subdirectory("ExternalMediaPlayerAdapters")
endif()

add_library(ExternalMediaPlayer SHARED
        ${ExternalMediaPlayer_SOURCES})

target_include_directories(ExternalMediaPlayer PUBLIC
    "${ExternalMediaPlayer_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(ExternalMediaPlayer
    AVSCommon)

if(HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    target_link_libraries(ExternalMediaPlayer ExternalMediaPlayerAdapters)
endif()


# install target
asdk_install()
