add_definitions("-DACSDK_LOG_MODULE=acsdkLibcurlHTTPContentFetcher")
add_library(acsdkLibcurlHTTPContentFetcher
    HTTPContentFetcherComponent.cpp)

target_include_directories(acsdkLibcurlHTTPContentFetcher PUBLIC
    "${acsdkLibcurlHTTPContentFetcher_SOURCE_DIR}/include")

target_link_libraries(acsdkLibcurlHTTPContentFetcher
    acsdkManufactory
    AVSCommon)

# install target
asdk_install()
