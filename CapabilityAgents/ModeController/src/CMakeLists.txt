add_definitions("-DACSDK_LOG_MODULE=modeController")

add_library(ModeController SHARED
    ModeControllerAttributeBuilder.cpp
    ModeControllerCapabilityAgent.cpp)

target_include_directories(ModeController
    PUBLIC "${ModeController_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(ModeController
    AVSCommon)

# install target
asdk_install()