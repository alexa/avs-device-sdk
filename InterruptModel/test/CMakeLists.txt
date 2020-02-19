cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${InterruptModel_SOURCE_DIR}/include"
        "${InterruptModel_SOURCE_DIR}/config"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
        )
set(TEST_FOLDER "${InterruptModel_SOURCE_DIR}/test")


discover_unit_tests("${INCLUDE_PATH}" "InterruptModel" "${TEST_FOLDER}")
