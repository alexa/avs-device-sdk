cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(FeatureClients LANGUAGES CXX)

add_subdirectory(AlexaPresentationFeatureClient)
add_subdirectory(PresentationOrchestratorFeatureClient)
add_subdirectory(VisualCharacteristicsFeatureClient)
add_subdirectory(VisualStateTrackerFeatureClient)

if (RTCSC)
    add_subdirectory(LiveViewControllerFeatureClient)
endif()
