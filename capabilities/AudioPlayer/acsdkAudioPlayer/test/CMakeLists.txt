cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
	"${acsdkAudioPlayer_INCLUDE_DIRS}"
	"${AVSCommon_SOURCE_DIR}/Utils/test"
	"${CaptionsTestLib_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "acsdkAudioPlayer;CaptionsTestLib;UtilsCommonTestLib;AVSCommon;SDKInterfacesTests;acsdkCryptoInterfacesTestLib;acsdkCrypto")
