add_definitions("-DACSDK_LOG_MODULE=contextManager")
add_library(ContextManager ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    ContextManager.cpp)

target_include_directories(ContextManager PUBLIC
    "${AVSCommon_INCLUDE_DIRS}"
    "${ContextManager_SOURCE_DIR}/include")

target_link_libraries(ContextManager
    AVSCommon)

# install target
asdk_install()