cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${InterruptModel_SOURCE_DIR}/include"
        "${InterruptModel_SOURCE_DIR}/config"
        )
set(TEST_FOLDER "${InterruptModel_SOURCE_DIR}/test")


discover_unit_tests("${INCLUDE_PATH}" "InterruptModel;SDKInterfacesTests" "${TEST_FOLDER}")
