cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(PKCS11 LANGUAGES CXX)

add_library(pkcs11-api-2.40 INTERFACE)
target_include_directories(pkcs11-api-2.40 INTERFACE "include")

add_library(pkcs11-api ALIAS pkcs11-api-2.40)
