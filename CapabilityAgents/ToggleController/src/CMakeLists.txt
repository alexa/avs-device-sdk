add_definitions("-DACSDK_LOG_MODULE=toggleController")

add_library(ToggleController SHARED
    ToggleControllerAttributeBuilder.cpp
    ToggleControllerCapabilityAgent.cpp)

target_include_directories(ToggleController
    PUBLIC "${ToggleController_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(ToggleController
    AVSCommon)

# install target
asdk_install()