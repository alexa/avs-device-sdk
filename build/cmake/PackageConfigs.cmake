#
# This module contains variables that define requirements of packages used in the SDK. Keep in mind that these variables
# are intended to be used with `find_package` function of CMAKE, e.g.
#       find_package(MyPackage ${MYPACKAGE_PACKAGE_CONFIG})
# where the variable name should be all capitalized. Therefore when adding new variables here, make sure that it follows
# the syntax of `find_package` that can be found here: @see https://cmake.org/cmake/help/v3.0/command/find_package.html
# 

# Set the minimum version required by CURL.
set(CURL_PACKAGE_CONFIG 7.43.0 REQUIRED)

# Make the Threads package required.
set(THREADS_PACKAGE_CONFIG REQUIRED)

# Minimum version of OpenSSL required
set(OPENSSL_MIN_VERSION 1.0.2)
