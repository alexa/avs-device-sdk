cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(LIBS
        "AttachmentCommonTestLib"
        "acsdkNotifications"
        "UtilsCommonTestLib"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")