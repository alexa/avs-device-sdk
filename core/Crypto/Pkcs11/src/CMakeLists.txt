
set(acsdkPkcs11_SOURCES
        KeyStoreFactory.cpp
        PKCS11Config.cpp
        PKCS11Functions.cpp
        PKCS11Key.cpp
        PKCS11KeyDescriptor.cpp
        PKCS11KeyStore.cpp
        PKCS11Slot.cpp
        PKCS11Session.cpp
        )

if(WIN32)
    list(APPEND acsdkPkcs11_SOURCES PKCS11FunctionsUwp.cpp)
else()
    list(APPEND acsdkPkcs11_SOURCES PKCS11FunctionsPosix.cpp)
endif()

add_library(acsdkPkcs11 ${acsdkPkcs11_SOURCES})
target_compile_definitions(acsdkPkcs11 PRIVATE ACSDK_LOG_MODULE=acsdkPkcs11)
target_compile_definitions(acsdkPkcs11 PUBLIC HAS_ACSDK_PKCS11)
target_include_directories(acsdkPkcs11 PUBLIC "${acsdkPkcs11_SOURCE_DIR}/include")
target_include_directories(acsdkPkcs11 PRIVATE "${acsdkPkcs11_SOURCE_DIR}/privateInclude")
target_link_libraries(acsdkPkcs11 PUBLIC acsdkCryptoInterfaces AVSCommon)
target_link_libraries(acsdkPkcs11 PRIVATE pkcs11-api-2.40)
if(UNIX)
    target_link_libraries(acsdkPkcs11 PRIVATE dl)
endif(UNIX)
# install target
asdk_install()
