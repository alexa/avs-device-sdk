cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkAlexaLiveViewController_SOURCE_DIR}/include"
    "${acsdkAlexaLiveViewController_SOURCE_DIR}/privateInclude"
    "${acsdkAlexaLiveViewControllerInterfaces_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}")

set(LIBS
        "AttachmentCommonTestLib"
        "acsdkAlexaLiveViewController"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
