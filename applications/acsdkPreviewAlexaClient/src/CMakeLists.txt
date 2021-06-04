set(LibPreviewAlexaClient_SOURCES)
list(APPEND LibPreviewAlexaClient_SOURCES
        PreviewAlexaClient.cpp
        PreviewAlexaClientComponent.cpp)

add_library(LibPreviewAlexaClient SHARED ${LibPreviewAlexaClient_SOURCES})

target_include_directories(LibPreviewAlexaClient PUBLIC
        "${acsdkPreviewAlexaClient_SOURCE_DIR}/include")

target_link_libraries(LibPreviewAlexaClient
        ${ACSDKALEXACOMMUNICATIONS_LIB}
        ${ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_LIB}
        ${ACSDKAUTHORIZATIONDELEGATE_LIB}
        ${ACSDKBLUETOOTHIMPLEMENTATIONS_LIB}
        ${ACSDKDEVICESETTINGSMANAGER_LIB}
        ${ACSDKHTTPCONTENTFETCHER_LIB}
        ${ACSDKINTERNETCONNECTIONMONITOR_LIB}
        ${ACSDKMETRICRECORDER_LIB}
        ${ACSDKSPEECHENCODER_LIB}
        ${ACSDKSYSTEMTIMEZONE_LIB}
        acsdkAlerts
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkCore
        acsdkDeviceSetup
        acsdkDeviceSetupInterfaces
        acsdkInteractionModel
        acsdkLibcurlHTTPContentFetcher
        acsdkManufactory
        acsdkSampleApplicationInterfaces
        LibSampleApp)

if (EXTERNAL_MEDIA_ADAPTERS)
    target_link_libraries(LibPreviewAlexaClient acsdkExternalMediaPlayerAdapters)
endif()

add_rpath_to_target("LibPreviewAlexaClient")

add_executable(PreviewAlexaClient
        previewMain.cpp)

target_link_libraries(PreviewAlexaClient LibPreviewAlexaClient)

# install target
asdk_install_targets(LibPreviewAlexaClient TRUE)
