add_definitions("-DACSDK_LOG_MODULE=acsdkLiveViewController")

add_library(acsdkAlexaLiveViewController
        "AlexaLiveViewControllerCapabilityAgent.cpp"
        "AlexaLiveViewControllerFactory.cpp")

target_include_directories(acsdkAlexaLiveViewController PRIVATE
        "${acsdkAlexaLiveViewController_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaLiveViewController PUBLIC
        "${acsdkAlexaLiveViewController_SOURCE_DIR}/include"
        "${ASDK_INCLUDE_DIRS}"
        ${RTCSC_INCLUDE_DIR})

target_link_libraries(acsdkAlexaLiveViewController
        acsdkAlexaLiveViewControllerInterfaces
        AVSCommon
        ${RTCSC_LIB_PATH})

# install target
asdk_install()