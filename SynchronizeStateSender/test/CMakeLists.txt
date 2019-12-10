set(INCLUDE_PATH
    "${SynchronizeStateSender_SOURCE_DIR}/include"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

discover_unit_tests("${INCLUDE_PATH}" SynchronizeStateSender)
add_definitions("-DACSDK_LOG_MODULE=synchronizeStateSenderTest")
