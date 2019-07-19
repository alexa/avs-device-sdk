add_definitions("-DACSDK_LOG_MODULE=speechEncoder")

add_library(SpeechEncoder SHARED
	SpeechEncoder.cpp
)

target_include_directories(SpeechEncoder
	PUBLIC
		"${SpeechEncoder_SOURCE_DIR}/include"
)

target_link_libraries(SpeechEncoder
	PUBLIC
		AVSCommon
)

# install target
asdk_install()