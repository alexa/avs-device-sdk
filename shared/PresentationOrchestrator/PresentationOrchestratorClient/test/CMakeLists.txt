cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${acsdkPresentationOrchestratorClient_INCLUDE_DIR}"
        "${acsdkPresentationOrchestratorClient_SOURCE_DIR}/include"
        "${acsdkPresentationOrchestratorClient_SOURCE_DIR}/privateInclude"
        "${ASDK_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "acsdkPresentationOrchestratorClient;PresentationOrchestratorTestLib")
