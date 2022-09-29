add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaInputController")

add_library(acsdkAlexaInputController SHARED
        InputControllerCapabilityAgent.cpp
        InputControllerFactory.cpp)

target_include_directories(acsdkAlexaInputController PUBLIC
        "${acsdkAlexaInputController_SOURCE_DIR}/include")

target_include_directories(acsdkAlexaInputController PRIVATE
        "${acsdkAlexaInputController_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkAlexaInputController
        acsdkAlexaInputControllerInterfaces
        acsdkShutdownManagerInterfaces
        Alexa
        AVSCommon)

# install target
asdk_install()
