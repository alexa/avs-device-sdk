cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

if(BUILD_TESTING)
    add_library(acsdkAuthorizationTestUtils
        StubStorage.cpp
    )
    target_include_directories(acsdkAuthorizationTestUtils PUBLIC
        "${acsdkAuthorization_SOURCE_DIR}/test/include"

    )
    target_link_libraries(acsdkAuthorizationTestUtils
        acsdkAuthorization
        acsdkAuthorizationInterfaces)
endif()

set(UNITTEST_LIBS
    acsdkAuthorization
    acsdkAuthorizationInterfaces
    acsdkAuthorizationTestUtils
    acsdkCryptoInterfaces
    acsdkPropertiesInterfacesTestLib
    AttachmentCommonTestLib
    RegistrationManagerTestUtils
    SDKInterfacesTests
    UtilsCommonTestLib)

set(UNITTEST_INCLUDES "${acsdkAuthorization_SOURCE_DIR}/privateInclude")

discover_unit_tests("${UNITTEST_INCLUDES}" "${UNITTEST_LIBS}")
