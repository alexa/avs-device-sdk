cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${DoNotDisturbCA_INCLUDE_DIRS}")

set(LIBS
        "DoNotDisturbCA"
        "DeviceSettingsTests"
        "SDKInterfacesTests"
        "AttachmentCommonTestLib")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
