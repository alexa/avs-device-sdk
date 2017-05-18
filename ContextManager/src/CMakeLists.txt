add_library(ContextManager SHARED
    ContextManager.cpp)

target_include_directories(ContextManager PUBLIC
    "${AVSCommon_INCLUDE_DIRS}"
    "${ContextManager_SOURCE_DIR}/include")

target_link_libraries(ContextManager
    AVSCommon
    AVSUtils)
