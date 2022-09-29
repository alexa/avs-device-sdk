add_definitions("-DACSDK_LOG_MODULE=presentationOrchestratorClient")

add_library(acsdkPresentationOrchestratorClient SHARED
        "Presentation.cpp"
        "PresentationLifespanToTimeoutMapper.cpp"
        "PresentationOrchestratorClient.cpp"
        "PresentationOrchestratorClientFactory.cpp"
        "WindowManager.cpp")

target_include_directories(acsdkPresentationOrchestratorClient
        PUBLIC "${acsdkPresentationOrchestratorClient_SOURCE_DIR}/include"
        "${ASDK_INCLUDE_DIRS}")

target_include_directories(acsdkPresentationOrchestratorClient
        PRIVATE "${acsdkPresentationOrchestratorClient_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkPresentationOrchestratorClient "${ASDK_LDFLAGS}" AVSCommon acsdkPresentationOrchestratorInterfaces acsdkVisualCharacteristicsInterfaces)

# install target
asdk_install()