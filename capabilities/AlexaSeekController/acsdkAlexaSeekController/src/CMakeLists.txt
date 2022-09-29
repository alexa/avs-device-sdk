add_definitions("-DACSDK_LOG_MODULE=AlexaSeekController")

add_library(acsdkAlexaSeekController
        AlexaSeekControllerCapabilityAgent.cpp
        AlexaSeekControllerFactory.cpp)

target_include_directories(acsdkAlexaSeekController PRIVATE
        "${acsdkAlexaSeekController_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaSeekController PUBLIC
        "${acsdkAlexaSeekController_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaSeekController AVSCommon Alexa acsdkAlexaSeekControllerInterfaces)

# install target
asdk_install()