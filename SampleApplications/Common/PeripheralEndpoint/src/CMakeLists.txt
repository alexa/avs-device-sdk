add_definitions("-DACSDK_LOG_MODULE=acsdkSamplePeripheralEndpoint")

set(SamplePeripheralEndpoint_SOURCES)
list(APPEND SamplePeripheralEndpoint_SOURCES
    NullPeripheralEndpoint.cpp)

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
    list(APPEND SamplePeripheralEndpoint_SOURCES PeripheralEndpointPowerControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
   list(APPEND SamplePeripheralEndpoint_SOURCES PeripheralEndpointToggleControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
   list(APPEND SamplePeripheralEndpoint_SOURCES PeripheralEndpointRangeControllerHandler.cpp)
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
   list(APPEND SamplePeripheralEndpoint_SOURCES PeripheralEndpointModeControllerHandler.cpp)
endif()

add_library(acsdkSamplePeripheralEndpoint ${SamplePeripheralEndpoint_SOURCES})

target_include_directories(acsdkSamplePeripheralEndpoint PUBLIC
        "${acsdkSamplePeripheralEndpoint_SOURCE_DIR}/include")

target_link_libraries(acsdkSamplePeripheralEndpoint
    acsdkSampleConsole
    AVSCommon
)

# install target only if sample libraries are installed
asdk_sample_install()
