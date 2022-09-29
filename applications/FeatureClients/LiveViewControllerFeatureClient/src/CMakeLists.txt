cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=LiveViewControllerFeatureClient")
add_library(acsdkLiveViewControllerFeatureClient
        LiveViewControllerFeatureClient.cpp
        LiveViewControllerFeatureClientBuilder.cpp)

target_include_directories(acsdkLiveViewControllerFeatureClient PUBLIC
        "${acsdkLiveViewControllerFeatureClient_SOURCE_DIR}/include"
        ${RTCSC_INCLUDE_DIR})

target_link_libraries(acsdkLiveViewControllerFeatureClient
        acsdkAlexaLiveViewController
        acsdkAlexaLiveViewControllerInterfaces
        acsdkSDKClient
        acsdkShutdownManager
        Alexa
        RtcscCapabilityAgent
        ${RTCSC_LIB_PATH})

# install target
asdk_install()
