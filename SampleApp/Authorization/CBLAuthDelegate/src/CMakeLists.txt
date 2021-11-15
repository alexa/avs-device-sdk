find_package(Threads ${THREADS_PACKAGE_CONFIG})

add_definitions("-DACSDK_LOG_MODULE=cblAuthDelegate")

if (CRYPTO_FOUND AND PKCS11)
    message(STATUS
            "\nIMPORTANT! Default SQLiteCBLAuthDelegateStorage uses encryption with hardware security module. "
            "Check \"pkcs11Module\" section in configuration file.\n"
            "Your application's auth token storage MUST be encrypted and protected by hardware security module for "
            "certification.\n")
else()
    message(WARNING
            "\nWARNING! Default SQLiteCBLAuthDelegateStorage is NOT encrypted. Your application's auth token storage "
            "MUST be encrypted and protected by hardware security module for certification.\n")
endif()

add_library(CBLAuthDelegate
    CBLAuthDelegate.cpp
    SQLiteCBLAuthDelegateStorage.cpp)
target_include_directories(CBLAuthDelegate PUBLIC
    "${CBLAuthDelegate_SOURCE_DIR}/include")
target_include_directories(CBLAuthDelegate PRIVATE
    "${RAPIDJSON_INCLUDE_DIR}")
target_link_libraries(
    CBLAuthDelegate
    AVSCommon
    RegistrationManager
    SQLiteStorage
    acsdkAuthorization
    acsdkAuthorizationInterfaces
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

# install target
asdk_install()
