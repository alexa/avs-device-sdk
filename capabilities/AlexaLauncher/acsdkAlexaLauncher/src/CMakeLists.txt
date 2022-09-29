add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaLauncher")

add_library(acsdkAlexaLauncher
        AlexaLauncherCapabilityAgent.cpp
        AlexaLauncherFactory.cpp)

target_include_directories(acsdkAlexaLauncher PRIVATE
        "${acsdkAlexaLauncher_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaLauncher PUBLIC
        "${acsdkAlexaLauncher_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaLauncher acsdkAlexaLauncherInterfaces AVSCommon Alexa acsdkManufactory)

# install target
asdk_install()