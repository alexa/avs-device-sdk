add_definitions("-DACSDK_LOG_MODULE=modeController")

add_library(ModeController ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    ModeControllerAttributeBuilder.cpp
    ModeControllerCapabilityAgent.cpp)

target_include_directories(ModeController
    PUBLIC "${ModeController_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(ModeController
    AVSCommon)

# install target
asdk_install()