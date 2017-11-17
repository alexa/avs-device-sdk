add_library(AttachmentCommonTestLib
        Common.cpp)
target_include_directories(AttachmentCommonTestLib PUBLIC
        "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(AttachmentCommonTestLib
        AVSCommon
        gtest_main
        gmock_main)
