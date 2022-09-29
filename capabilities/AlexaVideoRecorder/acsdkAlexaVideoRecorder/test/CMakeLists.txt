cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${AVSCommon_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${AVSCommon_SOURCE_DIR}/Utils/test"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
    "${acsdkAlexaVideoRecorder_INCLUDE_DIR}"
    "${acsdkAlexaVideoRecorder_SOURCE_DIR}/include"
    "${acsdkAlexaVideoRecorderInterfaces_SOURCE_DIR}/include"
    "${acsdkAlexaVideoRecorder_SOURCE_DIR}/privateInclude"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
        "AttachmentCommonTestLib"
        "acsdkAlexaVideoRecorder"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
