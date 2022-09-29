cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaRecordController_INCLUDE_DIR}"
    "${acsdkAlexaRecordController_SOURCE_DIR}/privateInclude"
    "${acsdkAlexaRecordControllerInterfaces_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
    "AttachmentCommonTestLib"
    "acsdkAlexaRecordController"
    "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
