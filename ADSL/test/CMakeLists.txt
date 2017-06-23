cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_subdirectory("common")

set(INCLUDE_PATH "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

set(ADSL_TEST_LIBS ADSL ADSLTestCommon)
discover_unit_tests("${INCLUDE_PATH}" "${ADSL_TEST_LIBS}")
