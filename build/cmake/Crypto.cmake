#
# Custom crypto library usage.
#
# To build with a customized version of crypto, run the following command,
#     cmake <path-to-source>
#           -DCRYPTO_LIBRARY=<path-to-crypto-library>
#           -DCRYPTO_INCLUDE_DIR=<path-to-crypto-include-dir>
#

set(CRYPTO_LIBRARY "" CACHE FILEPATH "Crypto library path.")
set(CRYPTO_INCLUDE_DIR "" CACHE PATH "Crypto include directory.")

mark_as_advanced(CRYPTO_INCLUDE_DIR CRYPTO_LIBRARY)

if (("${CRYPTO_LIBRARY}" STREQUAL "") OR ("${CRYPTO_INCLUDE_DIR}" STREQUAL ""))
    find_package(PkgConfig)
    pkg_check_modules(CRYPTO REQUIRED libcrypto)
else()
    set(CRYPTO_LDFLAGS ${CRYPTO_LIBRARY})
    set(CRYPTO_INCLUDE_DIRS ${CRYPTO_INCLUDE_DIR})
    set(CRYPTO_FOUND true)
endif()
