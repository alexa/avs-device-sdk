cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE AVSCommonSourceFiles "${AVSCommon_SOURCE_DIR}/src/*.cpp")

add_library(AVSCommon SHARED "${AVSCommonSourceFiles}")

target_include_directories(AVSCommon PUBLIC "${AVSCommon_SOURCE_DIR}/include" "${AVSUtils_SOURCE_DIR}/include"
        "${ACL_SOURCE_DIR}/ACL/include" "${RAPIDJSON_INCLUDE_DIR}")
target_link_libraries(AVSCommon ACL AVSUtils)

