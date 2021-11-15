cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(ACLTransportCommonTestLib
        MockHTTP2Connection.cpp
        MockHTTP2Request.cpp
        TestMessageRequestObserver.cpp
        MockMimeResponseSink.cpp)
    target_include_directories(ACLTransportCommonTestLib PUBLIC
        "${ACL_SOURCE_DIR}/include"
        "${ACL_SOURCE_DIR}/test/Transport")
    target_link_libraries(ACLTransportCommonTestLib
        AVSCommon
        gmock_main)
endif()
