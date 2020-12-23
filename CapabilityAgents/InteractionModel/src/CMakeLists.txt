add_definitions("-DACSDK_LOG_MODULE=interactionModel")

add_library(InteractionModel ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
        InteractionModelCapabilityAgent.cpp)

target_include_directories(InteractionModel PUBLIC
        "${InteractionModel_SOURCE_DIR}/include")

target_link_libraries(InteractionModel AVSCommon)

# install target
asdk_install()
