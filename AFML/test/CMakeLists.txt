set(INCLUDE_PATH
    "${AFML_SOURCE_DIR}/include"
    "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

discover_unit_tests("${INCLUDE_PATH}" AFML)
