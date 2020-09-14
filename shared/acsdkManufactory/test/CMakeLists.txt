cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
include(${AVS_CORE}/build/BuildDefaults.cmake)

discover_unit_tests("${acsdkManufactory_INCLUDE_DIRS}" acsdkManufactory AVSCommon)
