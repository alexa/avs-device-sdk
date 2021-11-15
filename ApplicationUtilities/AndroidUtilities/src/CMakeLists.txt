cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=androidUtilities")

add_library(AndroidUtilities
        AndroidLogger.cpp
        AndroidSLESBufferQueue.cpp
        AndroidSLESEngine.cpp
        AndroidSLESObject.cpp
        AndroidSLESMicrophone.cpp)

target_include_directories(AndroidUtilities PUBLIC
    "${AndroidUtilities_SOURCE_DIR}/include"
    "${AudioResources_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(AndroidUtilities AVSCommon OpenSLES log)

# install target
asdk_install()
