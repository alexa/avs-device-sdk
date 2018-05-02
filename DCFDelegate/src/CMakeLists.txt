cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_package(CURL ${CURL_PACKAGE_CONFIG})

add_definitions("-DACSDK_LOG_MODULE=dcfDelegate")
add_library(DCFDelegate SHARED
    DCFDelegate.cpp)
target_include_directories(DCFDelegate PUBLIC
    ${DCFDelegate_SOURCE_DIR}/include)
target_include_directories(DCFDelegate PRIVATE
    ${RAPIDJSON_INCLUDE_DIR})
target_link_libraries(DCFDelegate AVSCommon Integration ${CURL_LIBRARIES})

# install target
asdk_install()