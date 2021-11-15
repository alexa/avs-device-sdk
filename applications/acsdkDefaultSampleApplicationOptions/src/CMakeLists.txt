add_definitions("-DACSDK_LOG_MODULE=acsdkDefaultSampleApplicationOptions")
add_library(acsdkDefaultSampleApplicationOptions
        DefaultSampleApplicationOptionsComponent.cpp)

target_include_directories(acsdkDefaultSampleApplicationOptions PUBLIC
    "${acsdkDefaultSampleApplicationOptions_SOURCE_DIR}/include")

target_link_libraries(acsdkDefaultSampleApplicationOptions
    acsdkCore
    acsdkManufactory
    acsdkNullMetricRecorder
    acsdkSampleApplicationInterfaces
    acsdkSampleApplicationCBLAuthRequester
    acsdkShared
    CBLAuthDelegate)

# install target
asdk_install()
