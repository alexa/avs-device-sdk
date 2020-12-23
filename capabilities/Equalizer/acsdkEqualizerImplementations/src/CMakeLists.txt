add_definitions("-DACSDK_LOG_MODULE=equalizer")

add_library(acsdkEqualizerImplementations ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
        EqualizerComponent.cpp
        EqualizerController.cpp
        EqualizerUtils.cpp
        EqualizerLinearBandMapper.cpp
        InMemoryEqualizerConfiguration.cpp
        MiscDBEqualizerStorage.cpp
        SDKConfigEqualizerConfiguration.cpp
    )

target_include_directories(acsdkEqualizerImplementations PUBLIC
    "${acsdkEqualizerImplementations_SOURCE_DIR}/include")

target_link_libraries(acsdkEqualizerImplementations
    AVSCommon
    acsdkEqualizerInterfaces
    acsdkManufactory
    )

# install target
asdk_install()