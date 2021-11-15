
set(acsdkProperties_SOURCES
        Asn1Helper.cpp
        Asn1Types.cpp
        EncryptionKeyPropertyCodec.cpp
        EncryptionKeyPropertyCodecState.cpp
        DataPropertyCodec.cpp
        DataPropertyCodecState.cpp
        EncryptedProperties.cpp
        EncryptedPropertiesFactories.cpp
        EncryptedPropertiesFactory.cpp
        ErrorCallbackSetter.cpp
        Logging.cpp
        MiscStorageAdapter.cpp
        MiscStorageProperties.cpp
        MiscStoragePropertiesFactory.cpp
        RetryExecutor.cpp
        SimpleMiscStorageUriMapper.cpp
        )
set(acsdkProperties_PUBLIC_LIBRARIES
        acsdkCryptoInterfaces
        acsdkPropertiesInterfaces
        )
set(acsdkProperties_PRIVATE_LIBRARIES
        AVSCommon
        acsdkCodecUtils
        ${CRYPTO_LDFLAGS}
        )
set(acsdkProperties_PUBLIC_DEFINES)
set(acsdkProperties_PRIVATE_INCLUDES
        "${acsdkProperties_SOURCE_DIR}/privateInclude"
        ${CRYPTO_INCLUDE_DIRS}
        )

add_library(acsdkProperties ${acsdkProperties_SOURCES})
target_compile_definitions(acsdkProperties PRIVATE ACSDK_LOG_MODULE=acsdkProperties)
target_compile_definitions(acsdkProperties PUBLIC ${acsdkProperties_PUBLIC_DEFINES})
target_include_directories(acsdkProperties PUBLIC "${acsdkProperties_SOURCE_DIR}/include")
target_include_directories(acsdkProperties PRIVATE "${acsdkProperties_PRIVATE_INCLUDES}")
target_link_libraries(acsdkProperties PUBLIC ${acsdkProperties_PUBLIC_LIBRARIES})
target_link_libraries(acsdkProperties PRIVATE ${acsdkProperties_PRIVATE_LIBRARIES})

# install target
asdk_install()
