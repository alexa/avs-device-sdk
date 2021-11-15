cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(TEST_INCLUDES
        "${acsdkCodecUtils_SOURCE_DIR}/privateInclude"
        "${acsdkCodecUtils_SOURCE_DIR}/src"
        )
set(TEST_LIBRIRIES
        acsdkCodecUtils
        )

add_definitions("-DACSDK_LOG_MODULE=acsdkCodecUtilsTest")
if(CRYPTO_FOUND)
    add_definitions("-DCRYPTO_FOUND")
endif()
discover_unit_tests("${TEST_INCLUDES}" "${TEST_LIBRIRIES}")
