add_definitions("-DACSDK_LOG_MODULE=speechSynthesizer")

add_library(SpeechSynthesizer SHARED
        SpeechSynthesizer.cpp)

set_target_properties(SpeechSynthesizer PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(SpeechSynthesizer PUBLIC
        "${ContextManager_INCLUDE_DIRS}"
        "${SpeechSynthesizer_SOURCE_DIR}/include")

target_link_libraries(SpeechSynthesizer AVSCommon)

# install target
asdk_install()
