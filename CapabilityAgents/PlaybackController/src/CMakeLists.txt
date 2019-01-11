add_definitions("-DACSDK_LOG_MODULE=playbackcontroller")

add_library(PlaybackController SHARED
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackRouter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackMessageRequest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PlaybackCommand.cpp")

set_target_properties(PlaybackController PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(PlaybackController
    PUBLIC "${PlaybackController_SOURCE_DIR}/include")

target_link_libraries(PlaybackController AVSCommon ContextManager)

# install target
asdk_install()