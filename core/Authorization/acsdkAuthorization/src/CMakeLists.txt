add_definitions("-DACSDK_LOG_MODULE=authorizationManager")

add_library(
    acsdkAuthorization SHARED
    AuthorizationManager.cpp
    AuthorizationManagerStorage.cpp
    LWA/LWAAuthorizationAdapter.cpp
    LWA/LWAAuthorizationConfiguration.cpp
    LWA/SQLiteLWAAuthorizationStorage.cpp
)

target_include_directories(acsdkAuthorization PUBLIC
    "${acsdkAuthorization_SOURCE_DIR}/include"
)

target_link_libraries(acsdkAuthorization
    acsdkAuthorizationInterfaces
    acsdkManufactory
    AVSCommon
    RegistrationManager
    RegistrationManagerInterfaces
    SQLiteStorage
)

# install target
asdk_install()
