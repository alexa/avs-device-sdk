cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaRemoteVideoPlayer_INCLUDE_DIR}"
    "${acsdkAlexaRemoteVideoPlayer_SOURCE_DIR}/include"
    "${acsdkAlexaRemoteVideoPlayer_SOURCE_DIR}/privateInclude"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
        "AttachmentCommonTestLib"
        "acsdkAlexaRemoteVideoPlayer"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")