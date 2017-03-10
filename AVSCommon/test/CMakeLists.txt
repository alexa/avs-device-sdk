cmake_minimum_required(VERSION 3.0)

set(INCLUDES "${AVSCommon_SOURCE_DIR}/include" "${AVSUtils_SOURCE_DIR}/include" "${ACL_SOURCE_DIR}/ACL/include"
        "${ACL_SOURCE_DIR}/test" "${RAPIDJSON_INCLUDE_DIR}")

discover_unit_tests("${INCLUDES}" AVSCommon)
