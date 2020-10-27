set(LibPreviewAlexaClient_SOURCES)
list(APPEND LibPreviewAlexaClient_SOURCES
        PreviewAlexaClient.cpp
        PreviewAlexaClientComponent.cpp)

add_library(LibPreviewAlexaClient SHARED ${LibPreviewAlexaClient_SOURCES})

target_include_directories(LibPreviewAlexaClient PUBLIC
        "${acsdkPreviewAlexaClient_SOURCE_DIR}/include")

target_link_libraries(LibPreviewAlexaClient
        ${ACSDKALEXACOMMUNICATIONS_LIB}
        ${ACSDKAUTHORIZATIONDELEGATE_LIB}
        ${ACSDKHTTPCONTENTFETCHER_LIB}
        ${ACSDKINTERNETCONNECTIONMONITOR_LIB}
        ${ACSDKMETRICRECORDER_LIB}
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkCore
        acsdkManufactory
        acsdkSampleApplicationInterfaces
        LibSampleApp)

if (EXTERNAL_MEDIA_ADAPTERS)
    target_link_libraries(LibPreviewAlexaClient acsdkExternalMediaPlayerAdapters)
endif()

if (ANDROID_MEDIA_PLAYER)
    target_link_libraries(LibPreviewAlexaClient acsdkAndroidApplicationAudioPipelineFactory)
elseif (GSTREAMER_MEDIA_PLAYER)
    target_link_libraries(LibPreviewAlexaClient acsdkGstreamerApplicationAudioPipelineFactory)
elseif (CUSTOM_MEDIA_PLAYER)
    target_link_libraries(LibPreviewAlexaClient acsdkCustomApplicationAudioPipelineFactory)
endif()

add_rpath_to_target("LibPreviewAlexaClient")

add_executable(PreviewAlexaClient
        previewMain.cpp)

target_link_libraries(PreviewAlexaClient LibPreviewAlexaClient)

# install target
asdk_install_targets(LibPreviewAlexaClient TRUE)