add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaKeypadController")

add_library(acsdkAlexaKeypadController
        AlexaKeypadControllerCapabilityAgent.cpp
        AlexaKeypadControllerFactory.cpp)

target_include_directories(acsdkAlexaKeypadController PRIVATE
        "${acsdkAlexaKeypadController_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaKeypadController PUBLIC
        "${acsdkAlexaKeypadController_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaKeypadController AVSCommon Alexa acsdkAlexaKeypadControllerInterfaces acsdkManufactory)

# install target
asdk_install()
