cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(AlexaCATestUtils INTERFACE)

target_include_directories(AlexaCATestUtils INTERFACE
        "${Alexa_SOURCE_DIR}/test/include")

set(INCLUDE_PATH
        "${Alexa_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/AVS/test")

discover_unit_tests("${INCLUDE_PATH}" "Alexa;SDKInterfacesTests")
