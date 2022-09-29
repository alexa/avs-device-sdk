cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=visualStateTrackerFeatureClient")
add_library(acsdkVisualStateTrackerFeatureClient
        VisualStateTrackerFeatureClient.cpp
        VisualStateTrackerFeatureClientBuilder.cpp)

target_include_directories(acsdkVisualStateTrackerFeatureClient PUBLIC
        "${acsdkVisualStateTrackerFeatureClient_SOURCE_DIR}/include")

target_link_libraries(acsdkVisualStateTrackerFeatureClient
        acsdkPresentationOrchestratorInterfaces
        acsdkPresentationOrchestratorStateTracker
        acsdkShutdownManager
        acsdkVisualCharacteristics
        AFML
        acsdkSDKClient)

# install target
asdk_install()
