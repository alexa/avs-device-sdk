cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_library(AIP SHARED
    AudioInputProcessor.cpp
    MessageRequest.cpp)
target_include_directories(AIP PUBLIC
    "${AIP_SOURCE_DIR}/include"
    "${AFML_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}"
    "${AVSUtils_INCLUDE_DIRS}")
target_link_libraries(AIP
    AVSCommon
    AVSUtils
    ADSL
    AFML)
