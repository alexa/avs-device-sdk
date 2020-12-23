add_definitions("-DACSDK_LOG_MODULE=equalizer")

add_library(acsdkEqualizer ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
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
