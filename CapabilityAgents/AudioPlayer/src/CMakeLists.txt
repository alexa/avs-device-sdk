add_definitions("-DACSDK_LOG_MODULE=audioplayer")
add_library(AudioPlayer SHARED
    AudioPlayer.cpp
    ProgressTimer.cpp)

set_target_properties(AudioPlayer PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(AudioPlayer PUBLIC
    "${AudioPlayer_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(AudioPlayer
    AVSCommon)

# install target
asdk_install()
