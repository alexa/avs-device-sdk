cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE AVSCommonSourceFiles "${AVSCommon_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE AVSSourceFiles "${AVSCommon_SOURCE_DIR}/AVS/src/*.cpp")
file(GLOB_RECURSE UtilsSourceFiles "${AVSCommon_SOURCE_DIR}/Utils/src/*.cpp")

add_library(AVSCommon SHARED "${AVSCommonSourceFiles}" "${AVSSourceFiles}" "${UtilsSourceFiles}")

target_include_directories(AVSCommon PUBLIC "${AVSCommon_SOURCE_DIR}/include" "${AVSCommon_SOURCE_DIR}/Utils/include"
        "${AVSCommon_SOURCE_DIR}/AVS/include" "${AVSCommon_SOURCE_DIR}/SDKInterfaces/include"
        "${AVSUtils_SOURCE_DIR}/include" "${RAPIDJSON_INCLUDE_DIR}")
target_link_libraries(AVSCommon AVSUtils)

