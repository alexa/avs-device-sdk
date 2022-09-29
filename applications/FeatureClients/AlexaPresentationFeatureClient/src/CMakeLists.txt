cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=AlexaPresentationFeatureClient")
add_library(acsdkAlexaPresentationFeatureClient
        AlexaPresentationFeatureClient.cpp
        AlexaPresentationFeatureClientBuilder.cpp)

target_include_directories(acsdkAlexaPresentationFeatureClient PUBLIC
        "${acsdkAlexaPresentationFeatureClient_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaPresentationFeatureClient
        acsdkAlexaPresentation
        acsdkAlexaPresentationAPL
        acsdkAlexaPresentationInterfaces
        acsdkAPLCapabilityCommonInterfaces
        acsdkSDKClient
        acsdkShutdownManager)

# install target
asdk_install()
