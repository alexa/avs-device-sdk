add_definitions("-DACSDK_LOG_MODULE=speechEncoder")

add_library(SpeechEncoder SHARED
	SpeechEncoder.cpp
)

set_target_properties(SpeechEncoder PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

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