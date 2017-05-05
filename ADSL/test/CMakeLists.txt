cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_subdirectory("common")

set(ADSL_TEST_LIBS ADSL ADSLTestCommon)
discover_unit_tests("${AVSCommon_SOURCE_DIR}/test" "${ADSL_TEST_LIBS}")
