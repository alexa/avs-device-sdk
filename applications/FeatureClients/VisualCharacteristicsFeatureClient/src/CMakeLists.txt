cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=visualCharacteristicsFeatureClient")
add_library(acsdkVisualCharacteristicsFeatureClient
        VisualCharacteristicsFeatureClient.cpp
        VisualCharacteristicsFeatureClientBuilder.cpp)

target_include_directories(acsdkVisualCharacteristicsFeatureClient PUBLIC
        "${acsdkVisualCharacteristicsFeatureClient_SOURCE_DIR}/include")

target_link_libraries(acsdkVisualCharacteristicsFeatureClient
        acsdkPresentationOrchestratorInterfaces
        acsdkShutdownManager
        acsdkVisualCharacteristics
        acsdkVisualCharacteristicsInterfaces
        acsdkSDKClient)

# install target
asdk_install()
