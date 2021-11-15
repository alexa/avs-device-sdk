add_definitions("-DACSDK_LOG_MODULE=interactionModel")

add_library(acsdkInteractionModel
        InteractionModelCapabilityAgent.cpp
        InteractionModelComponent.cpp
        InteractionModelNotifier.cpp)

target_include_directories(acsdkInteractionModel PRIVATE
        "${acsdkInteractionModel_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkInteractionModel PUBLIC
        "${acsdkInteractionModel_SOURCE_DIR}/include")

target_link_libraries(acsdkInteractionModel AVSCommon acsdkInteractionModelInterfaces acsdkManufactory acsdkNotifierInterfaces acsdkNotifier)

# install target
asdk_install()
