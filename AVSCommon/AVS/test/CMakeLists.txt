add_subdirectory("Attachment")
set(INCLUDE_PATH
    "${AVSCommon_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/AVS/test"
    "${MetricRecorder_INCLUDE_DIRS}")
discover_unit_tests("${INCLUDE_PATH}" "AVSCommon;AttachmentCommonTestLib;SDKInterfacesTests")

if (BUILD_TESTING)
    add_library(AVSCommonTestLib INTERFACE)
    target_include_directories(AVSCommonTestLib INTERFACE "${AVSCommon_SOURCE_DIR}/AVS/test")
endif()

if (BUILD_MOCKS)
    install(DIRECTORY "${AVSCommon_SOURCE_DIR}/AVS/test/AVSCommon" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}/include")
endif()