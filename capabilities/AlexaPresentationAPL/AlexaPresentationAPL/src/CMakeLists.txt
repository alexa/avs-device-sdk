add_definitions("-DACSDK_LOG_MODULE=${PROJECT_NAME}")

file(GLOB SOURCES "*.cpp")
add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC "${acsdkAlexaPresentationAPL_SOURCE_DIR}/include"
        PRIVATE "${acsdkAlexaPresentationAPL_SOURCE_DIR}/privateInclude"
        "${ASDK_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME} "${ASDK_LDFLAGS}" AVSCommon acsdkAPLCapabilityCommon acsdkAPLCapabilityCommonInterfaces)

# install target
asdk_install()