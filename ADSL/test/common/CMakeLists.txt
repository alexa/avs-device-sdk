cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(ADSLTestCommon
    MockDirectiveHandler.cpp)
target_include_directories(ADSLTestCommon PUBLIC
    "${ADSL_SOURCE_DIR}/test/common"
    "${ADSL_INCLUDE_DIRS}"
    "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(ADSLTestCommon
    ADSL
    AVSCommon
    gtest_main
    gmock_main)
