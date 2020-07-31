add_definitions("-DACSDK_LOG_MODULE=opusEncoderContext")

add_library(OpusEncoderContext SHARED
OpusEncoderContext.cpp)

find_path(OPUS_INCLUDE_DIR opus)
find_library(OPUS_LIBRARY opus)

if(NOT OPUS_LIBRARY)
	message(FATAL_ERROR "Cannot find libopus")
endif()

target_include_directories(OpusEncoderContext
	PUBLIC
		"${SpeechEncoder_SOURCE_DIR}/include"
		"${OpusEncoderContext_SOURCE_DIR}/include"
	PRIVATE
		${OPUS_INCLUDE_DIR}
)

target_link_libraries(OpusEncoderContext
	PUBLIC
		SpeechEncoder
		AVSCommon
	PRIVATE
		${OPUS_LIBRARY}
)

# install target
asdk_install()