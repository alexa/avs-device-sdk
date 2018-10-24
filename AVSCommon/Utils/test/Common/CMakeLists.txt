cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(UtilsCommonTestLib
  MockMediaPlayer.cpp
  MockHTTP2MimeRequestEncodeSource.cpp
  MockHTTP2MimeResponseDecodeSink.cpp
  Common.cpp
  MimeUtils.cpp
  TestableAttachmentManager.cpp
  TestableAttachmentWriter.cpp
  TestableMessageObserver.cpp)
target_include_directories(UtilsCommonTestLib PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
	"${AVSCommon_SOURCE_DIR}/Utils/test")
target_link_libraries(UtilsCommonTestLib
        AVSCommon
        gtest_main
        gmock_main)
