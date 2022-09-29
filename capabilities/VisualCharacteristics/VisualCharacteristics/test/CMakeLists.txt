cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${acsdkVisualCharacteristics_INCLUDE_DIR}"
        "${acsdkVisualCharacteristics_SOURCE_DIR}/include"
        "${acsdkVisualCharacteristics_SOURCE_DIR}/privateInclude"
        "${ASDK_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "acsdkVisualCharacteristics;SDKInterfacesTests;acsdkPresentationOrchestratorInterfaces;PresentationOrchestratorTestLib")
