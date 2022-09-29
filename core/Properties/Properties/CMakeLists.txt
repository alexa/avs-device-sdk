cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(acsdkProperties LANGUAGES CXX)

add_subdirectory("src")
add_subdirectory("test")
if (PKCS11 AND BUILD_SHARED_LIBS)
  add_subdirectory("testCrypto")
endif()
