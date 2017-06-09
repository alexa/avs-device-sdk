cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(INCLUDE_PATH
    "${AIP_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/test"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
    "${AVSCommon_SOURCE_DIR}/AVS/test")

discover_unit_tests("${INCLUDE_PATH}" AIP)
