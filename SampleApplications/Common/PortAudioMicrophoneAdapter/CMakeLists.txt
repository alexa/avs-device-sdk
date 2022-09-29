cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkPortAudioMicrophoneAdapter LANGUAGES CXX)

add_library(${PROJECT_NAME}
        src/PortAudioMicrophoneAdapter.cpp
        src/PortAudioMicrophoneWrapper.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC include PRIVATE privateInclude "${PORTAUDIO_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} PUBLIC AVSCommon AudioResources PRIVATE  "${PORTAUDIO_LIB_PATH}")
target_compile_definitions(${PROJECT_NAME} PRIVATE ACSDK_LOG_MODULE=${PROJECT_NAME})

# install target only if sample libraries are installed
asdk_sample_install()
