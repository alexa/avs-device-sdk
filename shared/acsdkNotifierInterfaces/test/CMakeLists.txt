cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(NotifierTestLib INTERFACE)

    target_include_directories(NotifierTestLib INTERFACE "${acsdkNotifierInterfaces_SOURCE_DIR}/test/include")

    target_link_libraries(NotifierTestLib INTERFACE acsdkNotifierInterfaces gmock_main)
endif()
