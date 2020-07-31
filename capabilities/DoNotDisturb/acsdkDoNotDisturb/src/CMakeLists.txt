add_definitions("-DACSDK_LOG_MODULE=acsdkDoNotDisturb")

add_library(acsdkDoNotDisturb SHARED
        DNDMessageRequest.cpp
        DNDSettingProtocol.cpp
        DoNotDisturbCapabilityAgent.cpp)

target_include_directories(acsdkDoNotDisturb PUBLIC
        "${acsdkDoNotDisturb_SOURCE_DIR}/include")

target_link_libraries(acsdkDoNotDisturb AVSCommon RegistrationManager DeviceSettings)

# install target
asdk_install()
