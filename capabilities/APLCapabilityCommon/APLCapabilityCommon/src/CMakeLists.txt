add_definitions("-DACSDK_LOG_MODULE=${PROJECT_NAME} ")

file(GLOB SOURCES "*.cpp")
add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC "${acsdkAPLCapabilityCommon_SOURCE_DIR}/include"
        PRIVATE "${acsdkAPLCapabilityCommon_SOURCE_DIR}/privateInclude"
        "${ASDK_INCLUDE_DIRS}")

target_compile_definitions(${PROJECT_NAME} PRIVATE RAPIDJSON_HAS_STDSTRING=1)

target_link_libraries(${PROJECT_NAME} "${ASDK_LDFLAGS}" AVSCommon acsdkAPLCapabilityCommonInterfaces acsdkNotifier acsdkPresentationOrchestratorInterfaces)

# install target
asdk_install()
