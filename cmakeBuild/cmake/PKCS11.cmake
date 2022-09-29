#
# This module contains variables for PKCS11 integration and tests.
#
# To build with PKCS11 support, include the following option on the cmake command line.
#     cmake <path-to-source> -DPKCS11=ON
#
# To support unit tests with non-default parameters, include the following arguments in cmake command line:
#     cmake <other arguments> \
#         -DPKCS11_TEST_LIBRARY=<full path to PKCS#11 library> \
#         -DPKCS11_TEST_USER_PIN=<user pin> \
#         -DPKCS11_TEST_MAIN_KEY_ALIAS=<make key alias> \
#         -DPKCS11_TEST_TOKEN_NAME=<test token name>
#

if(CMAKE_SYSTEM_NAME MATCHES "Android")
    option(PKCS11 "Enable PKCS11" OFF)
else()
    option(PKCS11 "Enable PKCS11" ON)
endif()

# acsdkPkcs11 requires an absolute path to PKCS11 library. We use stubs in unit tests by default, and the default
# library location corresponds to project directory, but the location can be overridden.
set(PKCS11_TEST_LIBRARY_DIR "${PROJECT_BINARY_DIR}/core/Crypto/Pkcs11/testStubs/")
if (WIN32 AND NOT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" STREQUAL "")
    set(PKCS11_TEST_LIBRARY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
elseif(NOT WIN32 AND NOT "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" STREQUAL "")
    set(PKCS11_TEST_LIBRARY_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()
get_filename_component(PKCS11_TEST_LIBRARY_DIR "${PKCS11_TEST_LIBRARY_DIR}" ABSOLUTE)

set(PKCS11_TEST_LIBRARY
    "${PKCS11_TEST_LIBRARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}acsdkPkcs11Stubs${CMAKE_SHARED_LIBRARY_SUFFIX}"
    CACHE
    FILEPATH
    "PKCS11 Module Path for Unit Tests")
set(PKCS11_TEST_USER_PIN "1234" CACHE STRING "PKCS11 User Pin for Unit Tests" )
set(PKCS11_TEST_MAIN_KEY_ALIAS "TEST_KEY" CACHE STRING "PKCS11 Main Key Alias for Unit Tests" )
set(PKCS11_TEST_TOKEN_NAME "ACSDK" CACHE STRING "PKCS11 Token Name for Unit Tests")

mark_as_dependent(PKCS11_TEST_LIBRARY PKCS11)
mark_as_dependent(PKCS11_TEST_USER_PIN PKCS11)
mark_as_dependent(PKCS11_TEST_MAIN_KEY_ALIAS PKCS11)
mark_as_dependent(PKCS11_TEST_TOKEN_NAME PKCS11)

if (PKCS11)
    message(STATUS "PKCS11 Support is enabled")
    message(STATUS "\tModule Path: ${PKCS11_TEST_LIBRARY}")
    message(STATUS "\tToken Name: ${PKCS11_TEST_TOKEN_NAME}")
    message(STATUS "\tPIN: ${PKCS11_TEST_USER_PIN}")
    message(STATUS "\tMain Key: ${PKCS11_TEST_MAIN_KEY_ALIAS}")
else()
    message(STATUS "PKCS11 Support is disabled")
endif()
