cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(CertifiedSenderCommonTestLib
      MockCertifiedSender.cpp
    )
    target_include_directories(CertifiedSenderCommonTestLib PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
      "${AVSCommon_SOURCE_DIR}/Utils/test"
        "${CertifiedSender_SOURCE_DIR}/test")

    target_link_libraries(CertifiedSenderCommonTestLib
        AVSCommon
        SDKInterfacesTests
        CertifiedSender
        gtest_main
        gmock_main)
endif()
