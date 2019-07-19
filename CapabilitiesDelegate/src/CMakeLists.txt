cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_package(CURL ${CURL_PACKAGE_CONFIG})

add_definitions("-DACSDK_LOG_MODULE=capabilitiesDelegate")
add_library(CapabilitiesDelegate SHARED
    CapabilitiesDelegate.cpp)
target_include_directories(CapabilitiesDelegate PUBLIC
    ${CapabilitiesDelegate_SOURCE_DIR}/include)
target_include_directories(CapabilitiesDelegate PRIVATE
    ${RAPIDJSON_INCLUDE_DIR})
target_link_libraries(CapabilitiesDelegate AVSCommon ACL RegistrationManager ${CURL_LIBRARIES})

# install target
asdk_install()
