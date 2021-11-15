add_definitions("-DACSDK_LOG_MODULE=acsdkDoNotDisturb")

add_library(acsdkDoNotDisturb
        DNDMessageRequest.cpp
        DNDSettingProtocol.cpp
        DoNotDisturbCapabilityAgent.cpp
        DoNotDisturbComponent.cpp)

target_include_directories(acsdkDoNotDisturb PUBLIC
        "${acsdkDoNotDisturb_SOURCE_DIR}/include")

target_link_libraries(acsdkDoNotDisturb AVSCommon RegistrationManager DeviceSettings acsdkManufactory acsdkShutdownManagerInterfaces)

# install target
asdk_install()
