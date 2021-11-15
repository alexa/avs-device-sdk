set(LIBS
    "AVSCommon"
    "acsdkDavsClientForTesting"
    "acsdkAssetsMocks"
    )

discover_unit_tests("" "${LIBS}")