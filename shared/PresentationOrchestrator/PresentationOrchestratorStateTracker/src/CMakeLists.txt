add_definitions("-DACSDK_LOG_MODULE=presentationOrchestratorStateTracker")

add_library(acsdkPresentationOrchestratorStateTracker SHARED
        "PresentationOrchestratorStateTrackerFactory.cpp"
        "PresentationOrchestratorStateTracker.cpp")

target_include_directories(acsdkPresentationOrchestratorStateTracker
        PUBLIC "${acsdkPresentationOrchestratorStateTracker_SOURCE_DIR}/include"
        "${ASDK_INCLUDE_DIRS}")

target_include_directories(acsdkPresentationOrchestratorStateTracker
        PRIVATE "${acsdkPresentationOrchestratorStateTracker_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkPresentationOrchestratorStateTracker "${ASDK_LDFLAGS}" AFML AVSCommon acsdkNotifier acsdkPresentationOrchestratorInterfaces acsdkVisualCharacteristicsInterfaces)

# install target
asdk_install()