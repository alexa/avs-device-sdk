add_definitions("-DACSDK_LOG_MODULE=playbackcontroller")

add_library(PlaybackController ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackControllerComponent.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackRouter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackMessageRequest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackCommand.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackControllerComponent.cpp")

target_include_directories(PlaybackController
    PUBLIC "${PlaybackController_SOURCE_DIR}/include")

target_link_libraries(PlaybackController AVSCommon ContextManager acsdkManufactory acsdkShutdownManagerInterfaces)

# install target
asdk_install()