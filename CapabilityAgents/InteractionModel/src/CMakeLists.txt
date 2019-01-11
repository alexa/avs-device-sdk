add_definitions("-DACSDK_LOG_MODULE=interactionModel")

add_library(InteractionModel SHARED
        InteractionModelCapabilityAgent.cpp)

set_target_properties(InteractionModel PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(InteractionModel PUBLIC
        "${InteractionModel_SOURCE_DIR}/include")

target_link_libraries(InteractionModel AVSCommon)

# install target
asdk_install()
