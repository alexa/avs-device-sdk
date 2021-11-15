set(LIBS
        "AVSCommon"
        "acsdkCommunication"
        )

discover_unit_tests("${acsdkCommunication_INCLUDE_DIRS}" "${LIBS}")
