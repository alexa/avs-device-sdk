set(INCLUDE_PATH
        "${AVSCommon_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces")

set(LIBS
        "EqualizerImplementations"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
