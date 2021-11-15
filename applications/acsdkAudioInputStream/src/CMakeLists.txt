add_definitions("-DACSDK_LOG_MODULE=acsdkAudioInputStream")

add_library(acsdkAudioInputStream
        AudioInputStreamFactory.cpp
        AudioInputStreamComponent.cpp
        CompatibleAudioFormat.cpp)

target_include_directories(acsdkAudioInputStream PUBLIC
        "${acsdkAudioInputStream_SOURCE_DIR}/include")

target_link_libraries(acsdkAudioInputStream
        acsdkManufactory
        AVSCommon)

# install target
asdk_install()
