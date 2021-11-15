add_definitions("-DACSDK_LOG_MODULE=acsdkNullMetricRecorder")
add_library(acsdkNullMetricRecorder
    MetricRecorderComponent.cpp)

target_include_directories(acsdkNullMetricRecorder PUBLIC
    "${acsdkNullMetricRecorder_SOURCE_DIR}/include")

target_link_libraries(acsdkNullMetricRecorder
    acsdkManufactory)

# install target
asdk_install()
