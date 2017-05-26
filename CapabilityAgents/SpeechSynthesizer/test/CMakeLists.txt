cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(INCLUDE_PATH "${SpeechSynthesizer_INCLUDE_DIRS}" "${AVSCommon_SOURCE_DIR}/test"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

discover_unit_tests("${INCLUDE_PATH}" SpeechSynthesizer)