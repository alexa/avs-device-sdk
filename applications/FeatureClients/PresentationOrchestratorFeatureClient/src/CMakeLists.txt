cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=presentationOrchestratorFeatureClient")
add_library(acsdkPresentationOrchestratorFeatureClient
        PresentationOrchestratorFeatureClient.cpp
        PresentationOrchestratorFeatureClientBuilder.cpp)

target_include_directories(acsdkPresentationOrchestratorFeatureClient PUBLIC
        "${acsdkPresentationOrchestratorFeatureClient_SOURCE_DIR}/include")

target_link_libraries(acsdkPresentationOrchestratorFeatureClient
        acsdkPresentationOrchestratorClient
        acsdkPresentationOrchestratorInterfaces
        acsdkShutdownManager
        acsdkVisualTimeoutManager
        acsdkSDKClient)

# install target
asdk_install()
