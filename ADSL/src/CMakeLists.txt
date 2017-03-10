cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

add_library(ADSL SHARED
    DirectiveRouter.cpp
    DirectiveSequencer.cpp
    MessageInterpreter.cpp)
target_include_directories(ADSL PUBLIC
    "${ACL_INCLUDE_DIRS}"
    "${ADSL_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(ADSL
    AVSCommon
    AVSUtils)
