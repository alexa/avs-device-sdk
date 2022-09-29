add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaChannelController")

add_library(acsdkAlexaChannelController SHARED
        AlexaChannelControllerCapabilityAgent.cpp
        AlexaChannelControllerFactory.cpp)

target_include_directories(acsdkAlexaChannelController PRIVATE
        "${acsdkAlexaChannelController_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaChannelController PUBLIC
        "${acsdkAlexaChannelController_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaChannelController AVSCommon Alexa acsdkAlexaChannelControllerInterfaces)

# install target
asdk_install()
