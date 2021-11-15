set(INCLUDE_PATH
    "."
    "${acsdkAssetManager_SOURCE_DIR}/src"
    )

set(LIBS
    "AVSCommon"
    "acsdkAssetManagerForTesting"
    "acsdkAssetsMocks"
    )

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")