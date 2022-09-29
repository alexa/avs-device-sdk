add_definitions("-DACSDK_LOG_MODULE=visualTimeoutManager")

add_library(acsdkVisualTimeoutManager SHARED
        "${CMAKE_CURRENT_LIST_DIR}/VisualTimeoutManagerFactory.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/VisualTimeoutManager.cpp")

target_include_directories(acsdkVisualTimeoutManager
        PUBLIC "${acsdkVisualTimeoutManager_SOURCE_DIR}/include"
        "${ASDK_INCLUDE_DIRS}")

target_include_directories(acsdkVisualTimeoutManager
        PRIVATE "${acsdkVisualTimeoutManager_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkVisualTimeoutManager "${ASDK_LDFLAGS}" AVSCommon acsdkPresentationOrchestratorInterfaces)

# install target
asdk_install()