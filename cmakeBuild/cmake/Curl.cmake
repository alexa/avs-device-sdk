#
# Custom curl library usage.
#
# To build with a customized version of curl, run the following command,
#     cmake <path-to-source>
#           -DCURL_LIBRARY=<path-to-curl-library>
#           -DCURL_INCLUDE_DIR=<path-to-curl-include-dir>
#

if (("${CURL_LIBRARY}" STREQUAL "") OR ("${CURL_INCLUDE_DIR}" STREQUAL ""))
    find_package(CURL ${CURL_PACKAGE_CONFIG})
else()
    set(CURL_LIBRARIES ${CURL_LIBRARY})
    set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
    set(CURL_FOUND true)
endif()
