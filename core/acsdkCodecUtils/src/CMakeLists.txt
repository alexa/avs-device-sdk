
set(acsdkCodecUtils_SOURCES
        Base64Common.cpp
        CodecsCommon.cpp
        Hex.cpp
        )
set(acsdkCodecUtils_COMPILE_DEFS
        ACSDK_LOG_MODULE=acsdkCodecUtils
        )
set(acsdkCodecUtils_INCLUDES
        "${acsdkCodecUtils_SOURCE_DIR}/include"
        )
set(acsdkCodecUtils_PRIVATE_INCLUDES
        "${acsdkCodecUtils_SOURCE_DIR}/privateInclude"
        )
set(acsdkCodecUtils_LIBRARIES)

if(CRYPTO_FOUND)
    # Use OpenSSL Crypto API for Base64 if present.
    message(STATUS "Using OpenSSL for Base64 in acsdkCodecUtils")
    list(APPEND acsdkCodecUtils_SOURCES Base64OpenSsl.cpp)
    list(APPEND acsdkCodecUtils_PRIVATE_INCLUDES ${CRYPTO_INCLUDE_DIRS})
    list(APPEND acsdkCodecUtils_LIBRARIES ${CRYPTO_LDFLAGS})
else()
    message(STATUS "Using custom base64 implementation in acsdkCodecUtils.")
    list(APPEND acsdkCodecUtils_SOURCES Base64Internal.cpp)
endif()

add_library(acsdkCodecUtils ${acsdkCodecUtils_SOURCES})
target_compile_definitions(acsdkCodecUtils PRIVATE ${acsdkCodecUtils_COMPILE_DEFS})
target_include_directories(acsdkCodecUtils PUBLIC ${acsdkCodecUtils_INCLUDES})
target_include_directories(acsdkCodecUtils PRIVATE ${acsdkCodecUtils_PRIVATE_INCLUDES})
target_link_libraries(acsdkCodecUtils ${acsdkCodecUtils_LIBRARIES})

# install target
asdk_install()
