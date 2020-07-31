add_definitions("-DACSDK_LOG_MODULE=acsdkCore")
add_library(acsdkCore SHARED
    CoreComponent.cpp)

target_include_directories(acsdkCore PUBLIC
    "${acsdkCore_SOURCE_DIR}/include")

target_link_libraries(acsdkCore
    acsdkManufactory
    acsdkShared
    AVSCommon
    ContextManager
    RegistrationManager)

# install target
asdk_install()
