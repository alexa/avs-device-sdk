cmake_minimum_required(VERSION 3.1)

if (BUILD_TESTING)
    add_library(PresentationOrchestratorTestLib INTERFACE)

    target_include_directories(PresentationOrchestratorTestLib INTERFACE "${acsdkPresentationOrchestratorInterfaces_SOURCE_DIR}/test")

    target_link_libraries(PresentationOrchestratorTestLib INTERFACE acsdkPresentationOrchestratorInterfaces gmock_main)
endif ()