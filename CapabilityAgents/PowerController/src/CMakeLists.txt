add_definitions("-DACSDK_LOG_MODULE=powerController")

add_library(PowerController ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
    PowerControllerCapabilityAgent.cpp)

target_include_directories(PowerController
    PUBLIC "${PowerController_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(PowerController
    AVSCommon)

# install target
asdk_install()