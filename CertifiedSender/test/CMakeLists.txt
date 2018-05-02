set(INCLUDE_PATH
    "${AVSCommon_INCLUDE_DIRS}"
    "${CertifiedSender_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

set(TEST_FOLDER "${CertifiedSender_SOURCE_DIR}/test")

discover_unit_tests("${INCLUDE_PATH}" "CertifiedSender" "${TEST_FOLDER}")