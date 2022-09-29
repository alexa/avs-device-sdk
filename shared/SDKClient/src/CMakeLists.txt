cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(acsdkSDKClient
        SDKClientBuilder.cpp
        SDKClientRegistry.cpp
        TypeRegistry.cpp
        )

target_compile_definitions(acsdkSDKClient PRIVATE ACSDK_LOG_MODULE=sdkClient)
target_include_directories(acsdkSDKClient PUBLIC
        "${acsdkSDKClient_SOURCE_DIR}/include")

target_link_libraries(acsdkSDKClient AVSCommon acsdkShutdownManager)

# install target
asdk_install()
