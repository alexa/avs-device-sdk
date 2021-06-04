set(INCLUDE_PATH
        "${AVSGatewayManager_SOURCE_DIR}/include"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces")

set(LIBS
        "AVSGatewayManager"
        "RegistrationManagerTestUtils"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
add_definitions("-DACSDK_LOG_MODULE=avsGatewayManagerTest")
