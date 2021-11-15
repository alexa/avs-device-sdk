add_definitions("-DACSDK_LOG_MODULE=apiGateway")

add_library(
        ApiGateway
        ApiGatewayCapabilityAgent.cpp
)

target_include_directories(ApiGateway PUBLIC
        "${ApiGateway_SOURCE_DIR}/include")

target_link_libraries(
        ApiGateway
        AVSCommon
)

# install target
asdk_install()
