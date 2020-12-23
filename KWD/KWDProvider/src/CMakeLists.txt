add_library(KeywordDetectorProvider ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    KeywordDetectorProvider.cpp)

target_include_directories(KeywordDetectorProvider PUBLIC
	"${KeywordDetectorProvider_SOURCE_DIR}/include")

target_link_libraries(KeywordDetectorProvider KWD AVSCommon)

if(SENSORY_KEY_WORD_DETECTOR)
    target_link_libraries(KeywordDetectorProvider SENSORY)
endif()

if(KITTAI_KEY_WORD_DETECTOR)
    target_link_libraries(KeywordDetectorProvider KITTAI)
endif()

# install target
asdk_install()