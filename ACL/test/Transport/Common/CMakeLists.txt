cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(ACLTransportCommonTestLib
        MockHTTP2Connection.cpp
        MockHTTP2Request.cpp
        MockMimeResponseSink.cpp)
target_include_directories(ACLTransportCommonTestLib PUBLIC
        "${ACL_SOURCE_DIR}/include" "${ACL_SOURCE_DIR}/test/Transport")
target_link_libraries(ACLTransportCommonTestLib
        AVSCommon
        gtest_main
        gmock_main)
