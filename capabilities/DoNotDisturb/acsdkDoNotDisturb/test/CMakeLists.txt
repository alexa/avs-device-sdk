cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkDoNotDisturb_INCLUDE_DIRS}")

set(LIBS
        "acsdkDoNotDisturb"
        "DeviceSettingsTests"
        "SDKInterfacesTests"
        "AttachmentCommonTestLib")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
