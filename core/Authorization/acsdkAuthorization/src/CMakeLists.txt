add_definitions("-DACSDK_LOG_MODULE=authorizationManager")

set(acsdkAuthorization_SOURCES
        AuthorizationManager.cpp
        AuthorizationManagerStorage.cpp
        LWA/LWAAuthorizationAdapter.cpp
        LWA/LWAAuthorizationConfiguration.cpp
        LWA/LWAAuthorizationStorage.cpp
        LWA/LWAStorageConstants.cpp
        LWA/LWAStorageDataMigration.cpp
        )

set(acsdkAuthorization_LIBRARIES
        acsdkAuthorizationInterfaces
        acsdkCryptoInterfaces
        acsdkManufactory
        acsdkProperties
        AVSCommon
        RegistrationManager
        RegistrationManagerInterfaces
        SQLiteStorage
        )

add_library(acsdkAuthorization ${acsdkAuthorization_SOURCES})
target_include_directories(acsdkAuthorization PUBLIC "${acsdkAuthorization_SOURCE_DIR}/include")
target_include_directories(acsdkAuthorization PRIVATE "${acsdkAuthorization_SOURCE_DIR}/privateInclude")
target_link_libraries(acsdkAuthorization PUBLIC ${acsdkAuthorization_LIBRARIES})

# install target
asdk_install()
