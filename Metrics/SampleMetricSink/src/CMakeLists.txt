add_library(SampleMetricSink SampleMetricSink.cpp)

target_include_directories(SampleMetricSink	PUBLIC
	"${SampleMetricSink_SOURCE_DIR}/include"
)

target_link_libraries(SampleMetricSink AVSCommon)

# install target
asdk_install()
