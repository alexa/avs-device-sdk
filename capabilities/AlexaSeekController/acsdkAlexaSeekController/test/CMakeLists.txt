cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaSeekController_INCLUDE_DIR}"
    "${acsdkAlexaSeekController_SOURCE_DIR}/privateInclude"
    "${acsdkAlexaSeekControllerInterfaces_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
    "AttachmentCommonTestLib"
    "acsdkAlexaSeekController"
    "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")

