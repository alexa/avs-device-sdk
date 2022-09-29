cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
  "${ExternalMediaPlayer_SOURCE_DIR}/acsdkExternalMediaPlayerInterfaces/test")

discover_unit_tests("${INCLUDE_PATH}" "acsdkExternalMediaPlayer;UtilsCommonTestLib;SDKInterfacesTests;CertifiedSenderCommonTestLib;ApplicationAudioPipelineFactoryTestLib;ShutdownManagerTestLib;StartupNotifierTestLib")
