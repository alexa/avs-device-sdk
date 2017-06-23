cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(ACLTransportCommonTestLib
        Common.cpp
        MimeUtils.cpp
        TestableAttachmentManager.cpp
        TestableAttachmentWriter.cpp
        TestableMessageObserver.cpp)
target_include_directories(ACLTransportCommonTestLib PUBLIC
        "${ACL_SOURCE_DIR}/include")
target_link_libraries(ACLTransportCommonTestLib
        AVSCommon
        gtest_main
        gmock_main)
