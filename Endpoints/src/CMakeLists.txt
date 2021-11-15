add_definitions("-DACSDK_LOG_MODULE=endpoints")

add_library(Endpoints
        Endpoint.cpp
        EndpointAttributeValidation.cpp
        EndpointBuilder.cpp
        EndpointRegistrationManager.cpp
        DefaultEndpointBuilder.cpp)

target_include_directories(Endpoints PUBLIC
        $<BUILD_INTERFACE:${Endpoints_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/SDKInterfaces/include>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/Utils/include>
        $<BUILD_INTERFACE:${AVSCommon_SOURCE_DIR}/AVS/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(Endpoints AVSCommon Alexa)

if (ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
        target_include_directories(Endpoints PUBLIC
                $<BUILD_INTERFACE:${PowerController_SOURCE_DIR}/include>)
        target_link_libraries(Endpoints PowerController)
endif()

if (ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
        target_include_directories(Endpoints PUBLIC
                $<BUILD_INTERFACE:${ToggleController_SOURCE_DIR}/include>)
        target_link_libraries(Endpoints ToggleController)
endif()

if (ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
        target_include_directories(Endpoints PUBLIC
                $<BUILD_INTERFACE:${RangeController_SOURCE_DIR}/include>)
        target_link_libraries(Endpoints RangeController)
endif()

if (ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
        target_include_directories(Endpoints PUBLIC
                $<BUILD_INTERFACE:${ModeController_SOURCE_DIR}/include>)
        target_link_libraries(Endpoints ModeController)
endif()

# install target
asdk_install()
