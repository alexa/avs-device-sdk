add_library(UplCalculator
	BaseUplCalculator.cpp
	MediaUplCalculator.cpp
	TtsUplCalculator.cpp
	UplMetricSink.cpp)

target_include_directories(UplCalculator	PUBLIC
	"${UplCalculator_SOURCE_DIR}/include"
)

target_link_libraries(UplCalculator AVSCommon)

# install target
asdk_install()
