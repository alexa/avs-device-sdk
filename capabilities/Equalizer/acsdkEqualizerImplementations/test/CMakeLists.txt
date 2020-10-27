set(INCLUDE_PATH
        "${AVSCommon_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces"
        "${acsdkEqualizerImplementations_INCLUDE_DIRS}"
        "${acsdkEqualizerInterfaces_INCLUDE_DIRS}")

set(LIBS
        "AVSCommon"
        "acsdkEqualizerImplementations"
        "acsdkEqualizerInterfaces"
        "acsdkEqualizerInterfacesTestUtils"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")