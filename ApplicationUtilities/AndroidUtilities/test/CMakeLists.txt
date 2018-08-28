cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${AndroidUtilities_INCLUDE_DIRS}"
    "${AndroidUtilities_SOURCE_DIR}/test"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

set(LIBRARIES AndroidUtilities AVSCommon)

discover_unit_tests("${INCLUDE_PATH}" "${LIBRARIES}")

