add_definitions("-DACSDK_LOG_MODULE=contextManager")
add_library(ContextManager SHARED
    ContextManager.cpp)
set_target_properties(ContextManager PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(ContextManager PUBLIC
    "${AVSCommon_INCLUDE_DIRS}"
    "${ContextManager_SOURCE_DIR}/include")

target_link_libraries(ContextManager
    AVSCommon)

# install target
asdk_install()