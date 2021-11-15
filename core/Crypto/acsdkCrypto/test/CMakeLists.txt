cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(TEST_INCLUDES
        "${acsdkCrypto_SOURCE_DIR}/privateInclude"
        "${CRYPTO_INCLUDE_DIRS}"
        )
set(TEST_LIBRIRIES
        acsdkCrypto acsdkCodecUtils ${CRYPTO_LDFLAGS}
        )

add_definitions("-DACSDK_LOG_MODULE=acsdkCryptoTest")
discover_unit_tests("${TEST_INCLUDES}" "${TEST_LIBRIRIES}")
