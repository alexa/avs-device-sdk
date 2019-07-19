add_definitions("-DACSDK_LOG_MODULE=system")

add_library(AVSSystem SHARED
    "${CMAKE_CURRENT_LIST_DIR}/EndpointHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SoftwareInfoSender.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SoftwareInfoSendRequest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SystemCapabilityProvider.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/UserInactivityMonitor.cpp")

if (A4B)
    target_sources(AVSSystem PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/RevokeAuthorizationHandler.cpp")
endif()

target_include_directories(AVSSystem
    PUBLIC "${AVSSystem_SOURCE_DIR}/include")

target_link_libraries(AVSSystem AVSCommon ACL)

# install target
asdk_install()
