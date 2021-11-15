cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=interruptModel")

add_library(InterruptModel
        InterruptModel.cpp
        "${InterruptModel_SOURCE_DIR}/config/src/InterruptModelConfiguration.cpp")

set(LibInterruptModel_INCLUDES)
list(APPEND LibInterruptModel_INCLUDES "${InterruptModel_SOURCE_DIR}/include")
list(APPEND LibInterruptModel_INCLUDES "${InterruptModel_SOURCE_DIR}/config/include")

target_include_directories(InterruptModel PUBLIC
        "${LibInterruptModel_INCLUDES}")

if (MSVC)
    target_include_directories(InterruptModel PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}/exports")

    add_definitions("-DIN_INTERRUPTMODEL")
endif()

target_link_libraries(InterruptModel AVSCommon)

# install target
asdk_install_multiple("${LibInterruptModel_INCLUDES}")
