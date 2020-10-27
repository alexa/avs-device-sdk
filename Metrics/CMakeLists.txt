cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Metrics LANGUAGES CXX)

if (METRICS)
    add_subdirectory("SampleMetricSink")
    add_subdirectory("MetricRecorder")
    add_subdirectory("UplCalculator")
endif()