add_definitions("-DACSDK_LOG_MODULE=visualCharacteristics")

add_library(acsdkVisualCharacteristics SHARED
        "VCConfigParser.cpp"
        "VisualCharacteristics.cpp"
        "VisualCharacteristicsFactory.cpp"
        "VisualCharacteristicsSerializer.cpp"
        "VisualCharacteristicsSerializerFactory.cpp")

target_include_directories(acsdkVisualCharacteristics PUBLIC
        "${acsdkVisualCharacteristics_SOURCE_DIR}/include"
        "${acsdkPresentationOrchestratorInterfaces_SOURCE_DIR}/include"
        "${AVSCommon_INCLUDE_DIRS}")

target_include_directories(acsdkVisualCharacteristics
        PRIVATE "${acsdkVisualCharacteristics_SOURCE_DIR}/privateInclude")

target_link_libraries(acsdkVisualCharacteristics AVSCommon acsdkVisualCharacteristicsInterfaces acsdkPresentationOrchestratorInterfaces)

# install target
asdk_install()
