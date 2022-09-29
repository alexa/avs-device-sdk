cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${AIP_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${DeviceSettings_SOURCE_DIR}/test")

discover_unit_tests("${INCLUDE_PATH}" "AIP;UtilsCommonTestLib;CertifiedSenderCommonTestLib;AVSSystem;SDKInterfacesTests;acsdkAudioEncoder")
