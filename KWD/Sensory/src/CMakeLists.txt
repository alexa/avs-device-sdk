add_definitions("-DACSDK_LOG_MODULE=sensoryKeywordDetector")
add_library(SENSORY SHARED
    SensoryKeywordDetector.cpp)

include_directories(SENSORY 
	"${SENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR}"
	"${KWD_SOURCE_DIR}/include" 
	"${SENSORY_SOURCE_DIR}/include")

target_link_libraries(SENSORY KWD AVSCommon "${SENSORY_KEY_WORD_DETECTOR_LIB_PATH}")
