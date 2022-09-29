cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_subdirectory("PresentationOrchestratorClient" "POClient")
add_subdirectory("PresentationOrchestratorInterfaces" "POInterfaces")
add_subdirectory("PresentationOrchestratorStateTracker" "POStateTracker")
add_subdirectory("VisualTimeoutManager")