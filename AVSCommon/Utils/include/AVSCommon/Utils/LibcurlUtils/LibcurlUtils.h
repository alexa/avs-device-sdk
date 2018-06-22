/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLUTILS_H_

#include <curl/curl.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * Prepare a CURL handle to require TLS based upon global configuration settings.
 *
 * The 'libCurlUtils' sub-component of the global configuration supports the following options:
 * - CURLOPT_CAPATH If present, specifies a value for the libcurl property CURLOPT_CAPATH.
 *
 * Here is an example configuration:
 * @code
 * {
 *     "libcurlUtils" : {
 *         "CURLOPT_CAPATH" : "/path/to/directory/with/ca/certificates"
 *     }
 *     // Other configuration nodes
 * }
 * @endcode
 *
 * @param handle The libcurl handle to prepare.
 * @return Whether the operation was successful.
 */
bool prepareForTLS(CURL* handle);

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_LIBCURLUTILS_H_
