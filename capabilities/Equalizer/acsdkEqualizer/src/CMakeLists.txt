add_definitions("-DACSDK_LOG_MODULE=equalizer")

add_library(acsdkEqualizer
        EqualizerCapabilityAgent.cpp)

target_include_directories(acsdkEqualizer PUBLIC
        "${acsdkEqualizer_SOURCE_DIR}/include")

target_link_libraries(acsdkEqualizer
    AVSCommon
    RegistrationManager
    acsdkEqualizerInterfaces
    acsdkEqualizerImplementations)

# install target
asdk_install()
