add_library(MetricRecorder SHARED MetricRecorder.cpp)

target_include_directories(MetricRecorder	PUBLIC
	"${MetricRecorder_SOURCE_DIR}/include"
)

target_link_libraries(MetricRecorder AVSCommon)

# install target
asdk_install()
