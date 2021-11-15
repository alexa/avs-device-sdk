cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${acsdkPkcs11_SOURCE_DIR}/include"
        "${acsdkPkcs11_SOURCE_DIR}/privateInclude"
        "${PKCS11_SOURCE_DIR}"
        )

add_definitions(
        -DACSDK_LOG_MODULE=acsdkPkcs11Test
        -DPKCS11_LIBRARY="${PKCS11_TEST_LIBRARY}"
        -DPKCS11_PIN="${PKCS11_TEST_USER_PIN}"
        -DPKCS11_KEY_NAME="${PKCS11_TEST_MAIN_KEY_ALIAS}"
        -DPKCS11_TOKEN_NAME="${PKCS11_TEST_TOKEN_NAME}"
)

discover_unit_tests("${INCLUDE_PATH}" "acsdkPkcs11;pkcs11-api;AVSCommon;acsdkCrypto;acsdkPkcs11Stubs" ".")
