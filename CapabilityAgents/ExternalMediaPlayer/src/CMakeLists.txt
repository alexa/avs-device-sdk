add_definitions("-DACSDK_LOG_MODULE=ExternalMediaPlayer")
set(ExternalMediaPlayer_SOURCES )
list(APPEND ExternalMediaPlayer_SOURCES ExternalMediaPlayer.cpp)
list(APPEND ExternalMediaPlayer_SOURCES AdapterUtils.cpp)

add_library(ExternalMediaPlayer SHARED
        ${ExternalMediaPlayer_SOURCES})

target_include_directories(ExternalMediaPlayer PUBLIC
    "${ExternalMediaPlayer_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(ExternalMediaPlayer
    AVSCommon)

# install target
asdk_install()
