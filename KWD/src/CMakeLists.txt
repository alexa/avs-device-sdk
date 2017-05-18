add_library(KWD SHARED
    AbstractKeywordDetector.cpp)
    
include_directories(KWD "${KWD_SOURCE_DIR}/include" "${AVSCommon_SOURCE_DIR}/include")
target_link_libraries(KWD AVSCommon AVSUtils)