cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        $<BUILD_INTERFACE:${Endpoints_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/SDKInterfaces/include>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/SDKInterfaces/test>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/Utils/include>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/Utils/test>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/AVS/include>
        $<INSTALL_INTERFACE:include>)

discover_unit_tests("${INCLUDE_PATH}" "AlexaCATestUtils;UtilsCommonTestLib;Endpoints")
