cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkTemplateRuntime_SOURCE_DIR}/privateInclude"
    "${RAPIDJSON_INCLUDE_DIR}"
    "${AVSCommon_SOURCE_DIR}/AVS/test")

set(LIBS "acsdkTemplateRuntime" "SDKInterfacesTests" "AttachmentCommonTestLib")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
