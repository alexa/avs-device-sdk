add_definitions("-DACSDK_LOG_MODULE=alexa")

add_library(
        Alexa
        AlexaEventProcessedNotifier.cpp
        AlexaInterfaceCapabilityAgent.cpp
        AlexaInterfaceMessageSender.cpp
)

target_include_directories(Alexa PUBLIC
        "${Alexa_SOURCE_DIR}/include")

target_link_libraries(
        Alexa
        AVSCommon
        acsdkAlexaEventProcessedNotifierInterfaces
        acsdkNotifier
        acsdkManufactory
        acsdkShutdownManagerInterfaces
)

# install target
asdk_install()
