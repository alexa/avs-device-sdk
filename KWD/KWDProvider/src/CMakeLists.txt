add_library(KeywordDetectorProvider SHARED
    KeywordDetectorProvider.cpp)

target_include_directories(KeywordDetectorProvider PUBLIC
	"${KeywordDetectorProvider_SOURCE_DIR}/include")

target_link_libraries(KeywordDetectorProvider KWD AVSCommon)

if(SENSORY_KEY_WORD_DETECTOR)
    target_link_libraries(KeywordDetectorProvider SENSORY)
endif()

# install target
asdk_install()