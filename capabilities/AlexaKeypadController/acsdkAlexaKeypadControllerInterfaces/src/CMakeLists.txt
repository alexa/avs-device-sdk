add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaKeypadControllerInterfaces")

add_library(acsdkAlexaKeypadControllerInterfaces
        Keystroke.cpp)

target_include_directories(acsdkAlexaKeypadControllerInterfaces PUBLIC
        "${acsdkAlexaKeypadControllerInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaKeypadControllerInterfaces AVSCommon Alexa)

# install target
asdk_install()
