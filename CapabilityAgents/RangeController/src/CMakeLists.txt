add_definitions("-DACSDK_LOG_MODULE=rangeController")

add_library(RangeController
    RangeControllerAttributeBuilder.cpp
    RangeControllerCapabilityAgent.cpp)

target_include_directories(RangeController
    PUBLIC "${RangeController_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(RangeController
    AVSCommon)

# install target
asdk_install()