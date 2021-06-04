cmake_minimum_required(VERSION 3.1)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

add_library(RegistrationManagerTestUtils INTERFACE)
target_include_directories(RegistrationManagerTestUtils INTERFACE "${RegistrationManagerInterfaces_SOURCE_DIR}/test/include")
