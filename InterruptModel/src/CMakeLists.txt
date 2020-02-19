cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=interruptModel")

add_library(InterruptModel SHARED
        InterruptModel.cpp
        "${InterruptModel_SOURCE_DIR}/config/InterruptModelConfiguration.cpp")


target_include_directories(InterruptModel PUBLIC
        "${AVSCommon_SOURCE_DIR}/include"
        "${InterruptModel_SOURCE_DIR}/include"
        "${InterruptModel_SOURCE_DIR}/config")

target_link_libraries(InterruptModel AVSCommon)

# install target
asdk_install()
