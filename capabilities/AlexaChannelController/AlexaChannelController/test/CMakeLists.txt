cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaChannelController_SOURCE_DIR}/include"
    "${acsdkAlexaChannelController_SOURCE_DIR}/privateInclude"
    "${acsdkAlexaChannelControllerInterfaces_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}")

    set(LIBS
            "AttachmentCommonTestLib"
            "acsdkAlexaChannelController"
            "SDKInterfacesTests")

    discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
