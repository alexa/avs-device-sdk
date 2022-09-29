cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${acsdkPresentationOrchestratorStateTracker_INCLUDE_DIR}"
        "${acsdkPresentationOrchestratorStateTracker_SOURCE_DIR}/include"
        "${acsdkPresentationOrchestratorStateTracker_SOURCE_DIR}/privateInclude"
        "${ASDK_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "acsdkPresentationOrchestratorStateTracker")
