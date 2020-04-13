set(INCLUDE_PATH
    "${SynchronizeStateSender_SOURCE_DIR}/include")

discover_unit_tests("${INCLUDE_PATH}" "SynchronizeStateSender;SDKInterfacesTests")
add_definitions("-DACSDK_LOG_MODULE=synchronizeStateSenderTest")
