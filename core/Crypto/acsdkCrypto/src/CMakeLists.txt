
set(acsdkCrypto_SOURCES
        CryptoFactory.cpp
        OpenSslCryptoCodec.cpp
        OpenSslCryptoFactory.cpp
        OpenSslDigest.cpp
        OpenSslErrorCleanup.cpp
        OpenSslKeyFactory.cpp
        OpenSslTypeMapper.cpp
        OpenSslTypes.cpp
        )
set(acsdkCrypto_COMPILE_DEFS
        ACSDK_LOG_MODULE=acsdkCrypto
        )
set(acsdkCrypto_INCLUDES
        "${acsdkCrypto_SOURCE_DIR}/include"
        )
set(acsdkCrypto_PRIVATE_INCLUDES
        "${acsdkCrypto_SOURCE_DIR}/privateInclude"
        )
set(acsdkCrypto_LIBRARIES
        AVSCommon
        acsdkCryptoInterfaces
        ${CRYPTO_LDFLAGS}
        )

list(APPEND acsdkCrypto_LIBRARIES ${CRYPTO_LDFLAGS})
list(APPEND acsdkCrypto_PRIVATE_INCLUDES ${CRYPTO_INCLUDE_DIRS})

add_library(acsdkCrypto ${acsdkCrypto_SOURCES})
target_compile_definitions(acsdkCrypto PRIVATE ${acsdkCrypto_COMPILE_DEFS})
target_include_directories(acsdkCrypto PUBLIC ${acsdkCrypto_INCLUDES})
target_include_directories(acsdkCrypto PRIVATE ${acsdkCrypto_PRIVATE_INCLUDES})
target_link_libraries(acsdkCrypto ${acsdkCrypto_LIBRARIES})

# install target
asdk_install()
