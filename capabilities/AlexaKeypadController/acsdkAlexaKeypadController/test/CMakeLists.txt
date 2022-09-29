cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaKeypadController_SOURCE_DIR}/include"
    "${acsdkAlexaKeypadController_SOURCE_DIR}/privateInclude"
    "${acsdkAlexaKeypadControllerInterfaces_SOURCE_DIR}/include"
    "${AVSCommon_SOURCE_DIR}/AVS/test")

set(LIBS
    "acsdkAlexaKeypadController"
    "SDKInterfacesTests"
    "AttachmentCommonTestLib")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
