cmake_minimum_required(VERSION 3.1)

add_subdirectory("CryptoInterfaces")
add_subdirectory("Crypto")
if(PKCS11)
  add_subdirectory("Pkcs11")
endif()
