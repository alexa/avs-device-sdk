cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if(BUILD_TESTING)
    add_library(acsdkSpeakerManagerTestLib INTERFACE)
    target_include_directories(acsdkSpeakerManagerTestLib INTERFACE "${acsdkSpeakerManager_SOURCE_DIR}/test/include")

    set(INCLUDE_PATH "${acsdkSpeakerManager_SOURCE_DIR}/privateInclude")
    discover_unit_tests("${INCLUDE_PATH}" "acsdkSpeakerManager;UtilsCommonTestLib;SDKInterfacesTests;AVSCommonTestLib;acsdkSpeakerManagerTestLib")
endif()
