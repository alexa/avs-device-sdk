add_definitions("-DACSDK_LOG_MODULE=acsdkDefaultSampleApplicationOptions")
add_library(acsdkDefaultSampleApplicationOptions SHARED
        DefaultSampleApplicationOptionsComponent.cpp)

target_include_directories(acsdkDefaultSampleApplicationOptions PUBLIC
    "${acsdkDefaultSampleApplicationOptions_SOURCE_DIR}/include")

target_link_libraries(acsdkDefaultSampleApplicationOptions
    acsdkCore
    acsdkManufactory
    acsdkShared
    CBLAuthDelegate)

# install target
asdk_install()
