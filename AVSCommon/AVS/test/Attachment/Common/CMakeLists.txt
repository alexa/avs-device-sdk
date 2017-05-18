cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_library(AttachmentCommonTestLib
        Common.cpp)
target_include_directories(AttachmentCommonTestLib PUBLIC
        "${AVSCommon_SOURCE_DIR}/AVS/include")
target_link_libraries(AttachmentCommonTestLib
        AVSCommon
        gtest_main
        gmock_main)
