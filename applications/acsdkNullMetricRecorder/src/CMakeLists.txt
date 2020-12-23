add_definitions("-DACSDK_LOG_MODULE=acsdkNullMetricRecorder")
add_library(acsdkNullMetricRecorder ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    MetricRecorderComponent.cpp)

target_include_directories(acsdkNullMetricRecorder PUBLIC
    "${acsdkNullMetricRecorder_SOURCE_DIR}/include")

target_link_libraries(acsdkNullMetricRecorder
    acsdkManufactory)

# install target
asdk_install()
