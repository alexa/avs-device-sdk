add_definitions("-DACSDK_LOG_MODULE=alexa")

add_library(
        Alexa SHARED
        AlexaInterfaceCapabilityAgent.cpp
        AlexaInterfaceMessageSender.cpp
)

target_include_directories(Alexa PUBLIC
        "${Alexa_SOURCE_DIR}/include")

target_link_libraries(
        Alexa
        AVSCommon
)

# install target
asdk_install()
