cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkDeviceSetup_SOURCE_DIR}/include"
    "${acsdkDeviceSetup_SOURCE_DIR}/privateInclude"
    "${AVSCommon_SOURCE_DIR}/AVS/test")

set(LIBS
        "acsdkDeviceSetup"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
