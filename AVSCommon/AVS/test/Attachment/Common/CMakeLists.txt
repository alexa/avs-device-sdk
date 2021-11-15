if(BUILD_TESTING)
    add_library(AttachmentCommonTestLib
            Common.cpp)
    target_include_directories(AttachmentCommonTestLib PUBLIC
            "${AVSCommon_INCLUDE_DIRS}"
            "${AVSCommon_SOURCE_DIR}/AVS/test")
    target_link_libraries(AttachmentCommonTestLib
            AVSCommon
            gmock_main)
endif()