cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
if (BUILD_TESTING)
    set(CaptionsTestLib_SOURCES)
    list(APPEND CaptionsTestLib_SOURCES
        CaptionManagerTest.cpp
        CaptionTimingAdapterTest.cpp)
    if (CAPTIONS)
      list(APPEND CaptionsTestLib_SOURCES LibwebvttParserAdapterTest.cpp)
    endif()

    add_library(CaptionsTestLib ${CaptionsTestLib_SOURCES})

    target_include_directories(CaptionsTestLib PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/Utils/test"
        "${Captions_SOURCE_DIR}/Implementation/test")

    target_link_libraries(CaptionsTestLib
        AVSCommon
        CaptionsLib
        UtilsCommonTestLib
        SDKInterfacesTests
        gmock_main)

    set(INCLUDE_PATH
        "${AVSCommon_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/AVS/test"
        "${AVSCommon_SOURCE_DIR}/Utils/test")

    add_definitions("-DACSDK_LOG_MODULE=captionsTest")

    discover_unit_tests("${Captions}/Implementation/include" "CaptionsLib;UtilsCommonTestLib;SDKInterfacesTests")
endif()