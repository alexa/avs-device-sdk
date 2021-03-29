cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(ShutdownManagerTestLib INTERFACE)

    target_include_directories(ShutdownManagerTestLib INTERFACE "${acsdkShutdownManagerInterfaces_SOURCE_DIR}/test")

    target_link_libraries(ShutdownManagerTestLib INTERFACE acsdkShutdownManagerInterfaces gmock_main)
endif()
