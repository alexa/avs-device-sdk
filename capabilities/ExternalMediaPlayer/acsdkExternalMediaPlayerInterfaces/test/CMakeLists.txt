cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
  add_library(acsdkExternalMediaPlayerInterfacesTestLib INTERFACE)

  target_include_directories(acsdkExternalMediaPlayerInterfacesTestLib INTERFACE
    "${AVSCommon_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/Utils/test"
    "${acsdkExternalMediaPlayerInterfaces_SOURCE_DIR}/test")

  target_link_libraries(acsdkExternalMediaPlayerInterfacesTestLib INTERFACE
    AVSCommon
    SDKInterfacesTests
    acsdkExternalMediaPlayerInterfaces
    gmock_main)
endif()

set(INCLUDE_PATH
  "${acsdkExternalMediaPlayer_INCLUDE_DIRS}"
  "${AVSCommon_SOURCE_DIR}/Utils/test"
  "${CertifiedSender_SOURCE_DIR}/test")

discover_unit_tests("${INCLUDE_PATH}" "acsdkExternalMediaPlayer;UtilsCommonTestLib;SDKInterfacesTests;CertifiedSenderCommonTestLib;ApplicationAudioPipelineFactoryTestLib;ShutdownManagerTestLib")
