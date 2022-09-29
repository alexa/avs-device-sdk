add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaInputControllerInterfaces")

add_library(acsdkAlexaInputControllerInterfaces
        InputType.cpp)

target_include_directories(acsdkAlexaInputControllerInterfaces PUBLIC
        "${acsdkAlexaInputControllerInterfaces_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaInputControllerInterfaces AVSCommon Alexa)

# install target
asdk_install()
