cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${acsdkVisualTimeoutManager_INCLUDE_DIR}"
        "${acsdkVisualTimeoutManager_SOURCE_DIR}/include"
        "${acsdkVisualTimeoutManager_SOURCE_DIR}/privateInclude"
        "${ASDK_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "acsdkVisualTimeoutManager")
