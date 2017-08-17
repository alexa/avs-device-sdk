add_definitions("-DACSDK_LOG_MODULE=abstractKeywordDetector")
add_library(KWD SHARED
    AbstractKeywordDetector.cpp)

include_directories(KWD "${KWD_SOURCE_DIR}/include")
target_link_libraries(KWD AVSCommon)

# install target
asdk_install()