cmake_minimum_required(VERSION 3.1)

add_definitions("-DACSDK_LOG_MODULE=mediaPlayerTest")

set(INCLUDES
        "${AndroidSLESMediaPlayer_SOURCE_DIR}/include"
        "${AVSCommon_SOURCE_DIR}/AVS/test"
        "${AVSCommon_INCLUDE_DIRS}"
        "${AndroidUtilities_INCLUDE_DIRS}"
        "${AndroidUtilities_SOURCE_DIR}/test"
        "${PlaylistParser_SOURCE_DIR}/include"
        "${PlaylistParser_SOURCE_DIR}/test")

set(LIBRARIES AndroidSLESMediaPlayer AVSCommon AndroidUtilities)

set(INPUT_FOLDER "${AndroidSLESMediaPlayer_SOURCE_DIR}/../inputs")

discover_unit_tests("${INCLUDES}" "${LIBRARIES}" "${INPUT_FOLDER}")
