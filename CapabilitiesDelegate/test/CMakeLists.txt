set(INCLUDE_PATH
        "${CapabilitiesDelegate_SOURCE_DIR}/include")

discover_unit_tests("${INCLUDE_PATH}" "CapabilitiesDelegate;RegistrationManagerTestUtils;SDKInterfacesTests")
add_definitions("-DACSDK_LOG_MODULE=capabilitiesDelegateTest")

