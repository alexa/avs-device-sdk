cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(UNITTEST_INCLUDES
        "${acsdkProperties_SOURCE_DIR}/include"
        "${acsdkProperties_SOURCE_DIR}/privateInclude"
        "${AVSCommon_SOURCE_DIR}/Utils/test"
        )
set(UNITTEST_LIBRARIES
        SDKInterfacesTests
        acsdkProperties
        acsdkPropertiesInterfaces
        SDKInterfacesTests
        acsdkCodecUtils
        )

add_definitions(
        -DACSDK_LOG_MODULE=acsdkPropertiesTest
)

discover_unit_tests("${UNITTEST_INCLUDES}" "${UNITTEST_LIBRARIES}")
