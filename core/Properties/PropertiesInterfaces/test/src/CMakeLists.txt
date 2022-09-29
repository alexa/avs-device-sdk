cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(acsdkPropertiesInterfacesTestLib STATIC
            StubProperties.cpp
            StubPropertiesFactory.cpp)
    target_compile_definitions(acsdkPropertiesInterfacesTestLib PRIVATE ACSDK_LOG_MODULE=acsdkPropertiesInterfacesTest)
    target_include_directories(acsdkPropertiesInterfacesTestLib PUBLIC "../include")
    target_link_libraries(acsdkPropertiesInterfacesTestLib
            PUBLIC AVSCommonTestLib UtilsCommonTestLib
            PRIVATE acsdkPropertiesInterfaces acsdkCodecUtils)
endif()
