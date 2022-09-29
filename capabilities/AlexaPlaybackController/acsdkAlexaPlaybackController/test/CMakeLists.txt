cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaPlaybackController_INCLUDE_DIR}"
    "${acsdkAlexaPlaybackController_SOURCE_DIR}/privateInclude"
    "${acsdkAlexaPlaybackControllerInterfaces_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "SDKInterfacesTests;AttachmentCommonTestLib;acsdkAlexaPlaybackController")
