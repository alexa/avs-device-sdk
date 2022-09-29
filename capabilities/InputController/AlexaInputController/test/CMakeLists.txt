cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaInputController_INCLUDE_DIR}"
    "${acsdkAlexaInputController_SOURCE_DIR}/include"
    "${acsdkAlexaInputController_SOURCE_DIR}/privateInclude"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
    "AttachmentCommonTestLib"
    "acsdkAlexaInputController"
    "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
