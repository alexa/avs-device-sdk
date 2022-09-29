cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if (BUILD_TESTING)
    add_library(acsdkCryptoInterfacesTestLib INTERFACE)
    target_include_directories(acsdkCryptoInterfacesTestLib INTERFACE include)
    target_link_libraries(acsdkCryptoInterfacesTestLib INTERFACE acsdkCryptoInterfaces)
endif()
