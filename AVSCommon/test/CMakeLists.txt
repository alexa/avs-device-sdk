cmake_minimum_required(VERSION 3.0)

find_package(Threads ${THREADS_PACKAGE_CONFIG})

set(INCLUDES "${AVSCommon_SOURCE_DIR}/include" "${AVSUtils_SOURCE_DIR}/include" "${ACL_SOURCE_DIR}/ACL/include"
        "${ACL_SOURCE_DIR}/test" "${RAPIDJSON_INCLUDE_DIR}")

set(LIBRARIES AVSCommon ${CMAKE_THREAD_LIBS_INIT})

discover_unit_tests("${INCLUDES}" "${LIBRARIES}")
