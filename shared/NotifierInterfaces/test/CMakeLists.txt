cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(acsdkNotifierTestLib INTERFACE)

    target_include_directories(acsdkNotifierTestLib INTERFACE "${acsdkNotifierInterfaces_SOURCE_DIR}/test/include")

    target_link_libraries(acsdkNotifierTestLib INTERFACE acsdkNotifierInterfaces gmock_main)

    discover_unit_tests("" acsdkNotifierTestLib)
endif()
