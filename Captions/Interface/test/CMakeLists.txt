cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
if (BUILD_TESTING)
    add_library(CaptionsInterfaceTestLib
        CaptionDataTest.cpp
        CaptionFormatAvsTest.cpp
        CaptionFormatUtilsTest.cpp
        CaptionFrameTest.cpp
        CaptionLineTest.cpp
        TextStyleTest.cpp)

    target_include_directories(CaptionsInterfaceTestLib PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/Utils/test"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
        "${Captions_SOURCE_DIR}/Interface/include"
        "${Captions_SOURCE_DIR}/test")

    target_link_libraries(CaptionsInterfaceTestLib
        AVSCommon
        Captions
        UtilsCommonTestLib
        gmock_main)

    set(INCLUDE_PATH
        "${AVSCommon_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/AVS/test"
        "${AVSCommon_SOURCE_DIR}/Utils/test"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

    add_definitions("-DACSDK_LOG_MODULE=captionsInterfaceTest")

    discover_unit_tests("${Captions}/Interface/test" "Captions;UtilsCommonTestLib")
endif()
