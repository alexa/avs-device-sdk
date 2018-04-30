cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${RegistrationManager_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

discover_unit_tests("${INCLUDE_PATH}" RegistrationManager)
