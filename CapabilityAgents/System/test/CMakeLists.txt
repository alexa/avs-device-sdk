cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${AVSSystem_INCLUDE_DIR}"
    "${RAPIDJSON_INCLUDE_DIR}"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${DeviceSettings_SOURCE_DIR}/test")

discover_unit_tests("${INCLUDE_PATH}" "AVSSystem;ADSL;UtilsCommonTestLib;RegistrationManagerTestUtils;SDKInterfacesTests;ShutdownManagerTestLib")
