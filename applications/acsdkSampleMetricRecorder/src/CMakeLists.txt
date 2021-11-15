add_definitions("-DACSDK_LOG_MODULE=acsdkSampleMetricRecorder")
add_library(acsdkSampleMetricRecorder
        MetricRecorderComponent.cpp)

target_include_directories(acsdkSampleMetricRecorder PUBLIC
        "${acsdkSampleMetricRecorder_SOURCE_DIR}/include")

target_link_libraries(acsdkSampleMetricRecorder
        acsdkManufactory
        MetricRecorder
        SampleMetricSink)

# install target
asdk_install()
