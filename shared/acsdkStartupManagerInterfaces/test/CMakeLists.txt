cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
  add_library(StartupNotifierTestLib INTERFACE)

  target_include_directories(StartupNotifierTestLib INTERFACE "${acsdkStartupManagerInterfaces_SOURCE_DIR}/test")

  target_link_libraries(StartupNotifierTestLib INTERFACE acsdkStartupManagerInterfaces gmock_main)
endif()

