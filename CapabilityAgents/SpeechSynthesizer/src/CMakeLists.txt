add_library(SpeechSynthesizer SHARED
        SpeechSynthesizer.cpp)

target_include_directories(SpeechSynthesizer PUBLIC
        "${AVSCommon_SOURCE_DIR}/AVS"
        "${AVSCommon_INCLUDE_DIRS}"
        "${ContextManager_INCLUDE_DIRS}"
        "${AVSUtils_INCLUDE_DIRS}"
        "${ACL_SOURCE_DIR}/include"
        "${SpeechSynthesizer_SOURCE_DIR}/include")

target_link_libraries(SpeechSynthesizer
        AVSCommon
        AVSUtils
        ACL)