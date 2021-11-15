cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (${CRYPTO_FOUND})
    message(STATUS "Adding Crypto Implementation.")

    project(acsdkCrypto LANGUAGES CXX)
    add_subdirectory("src")
    add_subdirectory("test")
else()
    message(STATUS "Disabling Crypto Implementation.")
endif()
