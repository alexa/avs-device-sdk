cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(UNITTEST_INCLUDES
        "${acsdkProperties_SOURCE_DIR}/include"
        "${acsdkProperties_SOURCE_DIR}/privateInclude"
        "${AVSCommon_SOURCE_DIR}/Utils/test"
        "${CRYPTO_INCLUDE_DIRS}"
        )

set(UNITTEST_LIBRARIES
        acsdkProperties
        acsdkCrypto
        acsdkPkcs11
        acsdkCryptoInterfaces
        acsdkPropertiesInterfaces
        SDKInterfacesTests
        acsdkCryptoInterfacesTestLib
        acsdkPropertiesInterfacesTestLib
        acsdkCodecUtils
        acsdkPkcs11Stubs
        )

add_definitions(
        -DACSDK_LOG_MODULE=acsdkEncryptedPropertiesTest
        -DPKCS11_LIBRARY="${PKCS11_TEST_LIBRARY}"
        -DPKCS11_PIN="${PKCS11_TEST_USER_PIN}"
        -DPKCS11_KEY_NAME="${PKCS11_TEST_MAIN_KEY_ALIAS}"
        -DPKCS11_TOKEN_NAME="${PKCS11_TEST_TOKEN_NAME}"
)

discover_unit_tests("${UNITTEST_INCLUDES}" "${UNITTEST_LIBRARIES}")
