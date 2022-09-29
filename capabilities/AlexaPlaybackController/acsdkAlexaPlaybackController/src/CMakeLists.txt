add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaPlaybackController")

add_library(acsdkAlexaPlaybackController
        AlexaPlaybackControllerCapabilityAgent.cpp
        AlexaPlaybackControllerFactory.cpp)

target_include_directories(acsdkAlexaPlaybackController PRIVATE
        "${acsdkAlexaPlaybackController_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaPlaybackController PUBLIC
        "${acsdkAlexaPlaybackController_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaPlaybackController AVSCommon Alexa acsdkAlexaPlaybackControllerInterfaces acsdkManufactory)

# install target
asdk_install()
