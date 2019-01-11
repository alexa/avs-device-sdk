add_definitions("-DACSDK_LOG_MODULE=equalizer")

add_library(Equalizer SHARED
        EqualizerCapabilityAgent.cpp)

set_target_properties(Equalizer PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(Equalizer PUBLIC
        "${Equalizer_SOURCE_DIR}/include")

target_link_libraries(Equalizer AVSCommon RegistrationManager EqualizerImplementations)

# install target
asdk_install()
