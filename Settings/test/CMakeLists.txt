cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(DeviceSettingsTests INTERFACE)

    target_include_directories(DeviceSettingsTests INTERFACE "${DeviceSettings_SOURCE_DIR}/test")

    target_link_libraries(DeviceSettingsTests INTERFACE DeviceSettings SDKInterfacesTests gmock_main)

    set(INCLUDE_PATH
        "${DeviceSettings_SOURCE_DIR}/include"
        "${AVSCommon_SOURCE_DIR}/AVS/test"
        "${AVSCommon_SOURCE_DIR}/Utils/include"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
        "${AVSCommon_SOURCE_DIR}/Utils/test"
        "${CertifiedSender_SOURCE_DIR}/test")

    set(TEST_FOLDER "${DeviceSettings_SOURCE_DIR}/test")

    discover_unit_tests("${INCLUDE_PATH}" "DeviceSettings;DeviceSettingsTests;RegistrationManagerTestUtils" "${TEST_FOLDER}")

endif()
