cmake_minimum_required(VERSION 3.1 FATAL_ERROR)	
project(MultipartParser LANGUAGES CXX)

add_library(MultipartParser INTERFACE)

target_include_directories(MultipartParser INTERFACE .)

install(DIRECTORY MultipartParser
        DESTINATION ${ASDK_INCLUDE_INSTALL_DIR}/include
        FILES_MATCHING PATTERN "*.h")
