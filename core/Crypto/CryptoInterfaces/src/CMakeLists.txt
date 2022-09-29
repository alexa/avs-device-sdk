
add_library(acsdkCryptoInterfaces AlgorithmType.cpp DigestType.cpp)
target_compile_definitions(acsdkCryptoInterfaces PRIVATE ACSDK_LOG_MODULE=acsdkCryptoInterfaces)
target_include_directories(acsdkCryptoInterfaces PUBLIC ${acsdkCryptoInterfaces_SOURCE_DIR}/include)

# install target
asdk_install()
