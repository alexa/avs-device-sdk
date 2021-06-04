add_subdirectory("Common")

set(INCLUDE_PATH
        "${AVSCommon_INCLUDE_DIRS}"
        "${CertifiedSender_INCLUDE_DIRS}")

set(TEST_FOLDER "${CertifiedSender_BINARY_DIR}/test")

discover_unit_tests("${INCLUDE_PATH}" "CertifiedSender;SDKInterfacesTests;RegistrationManagerTestUtils" "${TEST_FOLDER}")