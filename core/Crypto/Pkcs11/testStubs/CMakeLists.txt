cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkPkcs11Stubs LANGUAGES CXX)

# PKCS11 requires code is loaded dynamically.
add_library(acsdkPkcs11Stubs SHARED src/Pkcs11Stubs.cpp)
target_compile_definitions(acsdkPkcs11Stubs PRIVATE ACSDK_LOG_MODULE=acsdkPkcs11Stubs)
target_link_libraries(acsdkPkcs11Stubs PRIVATE pkcs11-api-2.40 acsdkCrypto)

