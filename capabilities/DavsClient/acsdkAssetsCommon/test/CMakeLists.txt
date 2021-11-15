project(acsdkAssetsCommonTest LANGUAGES CXX)
add_subdirectory(mocks)

set(LIBS
    "AVSCommon"
    "acsdkAssetsCommon"
    "acsdkAssetsMocks"
    )

discover_unit_tests("" "${LIBS}")