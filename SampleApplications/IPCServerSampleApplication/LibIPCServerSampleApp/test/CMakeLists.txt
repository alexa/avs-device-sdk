cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${LibIPCServerSampleApp_INCLUDE_DIR}")

discover_unit_tests("${INCLUDE_PATH}" "LibIPCServerSampleApp")
