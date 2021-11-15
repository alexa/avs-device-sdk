add_definitions("-DACSDK_LOG_MODULE=DavsClient")

if (NOT LibArchive_FOUND)
    find_package(LibArchive)
endif()

add_library(acsdkAssetsCommon
    AmdMetricWrapper.cpp
    ArchiveWrapper.cpp
    Base64Url.cpp
    CurlWrapper.cpp
    DataChunk.cpp
    DownloadChunkQueue.cpp
    DownloadStream.cpp
    JitterUtil.cpp
    ResponseSink.cpp
    )

target_include_directories(acsdkAssetsCommon PUBLIC
    ${LibArchive_INCLUDE_DIRS}
    ${CRYPTO_INCLUDE_DIRS}
    ${acsdkAssetsCommon_SOURCE_DIR}/include
    )

target_link_libraries(acsdkAssetsCommon
    ${LibArchive_LIBRARIES}
    ${CRYPTO_LDFLAGS}
    AVSCommon
    acsdkAssetsInterfaces
    acsdkDavsClientInterfaces
    )

# install target
asdk_install()
