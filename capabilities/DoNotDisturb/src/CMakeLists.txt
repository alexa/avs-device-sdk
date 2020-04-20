add_definitions("-DACSDK_LOG_MODULE=doNoDisturbCA")

add_library(DoNotDisturbCA SHARED
        DNDMessageRequest.cpp
        DNDSettingProtocol.cpp
        DoNotDisturbCapabilityAgent.cpp)

target_include_directories(DoNotDisturbCA PUBLIC
        "${DoNotDisturbCA_SOURCE_DIR}/include")

target_link_libraries(DoNotDisturbCA AVSCommon RegistrationManager DeviceSettings)

# install target
asdk_install()
