include(CheckCXXCompilerFlag)
project(acsdkAssetsMocks LANGUAGES CXX)

add_library(acsdkAssetsMocks
    AuthDelegateMock.cpp
    CurlWrapperMock.cpp
    DavsServiceMock.cpp
    InternetConnectionMonitorMock.cpp
    TestUtil.cpp
    )

target_compile_definitions(acsdkAssetsMocks
    PUBLIC
    RUNS_ON_HOST=1
    PRIVATE
    ACSDK_LOG_MODULE=AssetsMock)

target_include_directories(acsdkAssetsMocks PUBLIC include)

target_link_libraries(acsdkAssetsMocks
    AVSCommon
    acsdkAssetsInterfaces
    acsdkAssetsCommon
    )

CHECK_CXX_COMPILER_FLAG("-Wno-deprecated-declarations" HAS_NO_DEPRECATED_DECLARATIONS)
if (HAS_NO_DEPRECATED_DECLARATIONS)
    target_compile_options(acsdkAssetsMocks PUBLIC
        -Wno-deprecated-declarations
        )
endif()
CHECK_CXX_COMPILER_FLAG("-Wno-attributes" HAS_NO_ATTRIBUTES)
if (HAS_NO_ATTRIBUTES)
    target_compile_options(acsdkAssetsMocks PUBLIC
        -Wno-attributes
        )
endif()

# install target
install(TARGETS acsdkAssetsMocks DESTINATION lib)
install(DIRECTORY include/
        DESTINATION include/acsdkAssetsMocks)
