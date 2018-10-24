cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${Alerts_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/AVS/test"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
        "${AVSCommon_SOURCE_DIR}/Utils/test")

discover_unit_tests("${INCLUDE_PATH}" "Alerts;UtilsCommonTestLib")
