add_definitions("-DACSDK_LOG_MODULE=ExternalMediaPlayer")

add_library(ExternalMediaPlayer SHARED
        AuthorizedSender.cpp
        ExternalMediaPlayer.cpp)

target_include_directories(ExternalMediaPlayer PUBLIC
    "${ExternalMediaPlayer_SOURCE_DIR}/include")

target_link_libraries(ExternalMediaPlayer
    AVSCommon CertifiedSender)

set(EXTERNAL_MEDIA_PLAYER_ADAPTER_TARGETS "" CACHE INTERNAL "")

# Add adapters and find linkable targets.
add_external_media_player_adapters("${EXTERNAL_MEDIA_PLAYER_ADAPTER_PATHS}")

foreach(ADAPTER IN LISTS ALL_EMP_ADAPTER_TARGETS)
    target_link_libraries(ExternalMediaPlayer ${ADAPTER})
endforeach()

# install target
asdk_install()
