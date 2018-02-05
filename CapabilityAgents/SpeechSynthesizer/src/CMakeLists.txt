add_definitions("-DACSDK_LOG_MODULE=speechSynthesizer")

add_library(SpeechSynthesizer SHARED
        SpeechSynthesizer.cpp)

target_include_directories(SpeechSynthesizer PUBLIC
        "${ContextManager_INCLUDE_DIRS}"
        "${SpeechSynthesizer_SOURCE_DIR}/include")

target_link_libraries(SpeechSynthesizer AVSCommon)

# install target
asdk_install()
