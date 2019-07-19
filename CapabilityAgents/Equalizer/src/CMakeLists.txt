add_definitions("-DACSDK_LOG_MODULE=equalizer")

add_library(Equalizer SHARED
        EqualizerCapabilityAgent.cpp)

target_include_directories(Equalizer PUBLIC
        "${Equalizer_SOURCE_DIR}/include")

target_link_libraries(Equalizer AVSCommon RegistrationManager EqualizerImplementations)

# install target
asdk_install()
