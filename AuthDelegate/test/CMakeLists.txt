cmake_minimum_required(VERSION 3.0)

include_directories("${CMAKE_SOURCE_DIR}/ACL/test/")
discover_unit_tests("${AuthDelegate_SOURCE_DIR}/include" AuthDelegate)
