cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
  
set(INCLUDE_PATH
    "${AVSCommon_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${AVSCommon_SOURCE_DIR}/Utils/test"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
    "${acsdkAPLCapabilityCommon_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
        "AVSCommon"
        "AVSCommonTestLib"
        "SDKInterfacesTests"
        "AttachmentCommonTestLib"
        "acsdkAPLCapabilityCommon"
        "acsdkPresentationOrchestratorInterfaces"
        "acsdkAPLCapabilityCommonInterfaces"
        "acsdkAPLCapabilityCommonInterfacesTestUtils"
)
discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
