add_definitions("-DACSDK_LOG_MODULE=kittAiKeyWordDetector")
add_library(KITTAI SHARED
    SnowboyWrapper.cpp
    KittAiKeyWordDetector.cpp)

target_include_directories(KITTAI PUBLIC
	"${KITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR}"
	"${KWD_SOURCE_DIR}/include"
	"${KITTAI_SOURCE_DIR}/include")

target_link_libraries(KITTAI KWD AVSCommon "${KITTAI_KEY_WORD_DETECTOR_LIB_PATH}" blas)

# install target
asdk_install()