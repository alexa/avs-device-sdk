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

#include <cerrno>
#include <cstdlib>

#include "AVSCommon/Utils/Configuration/ConfigurationNode.h"
#include "AVSCommon/Utils/LibcurlUtils/LibcurlUtils.h"
#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/// String to identify log entries originating from this file.
static const std::string TAG("LibcurlUtils");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Key for looking up the @c LibCurlUtil @c ConfigurationNode.
static const std::string LIBCURLUTILS_CONFIG_KEY = "libcurlUtils";

/// Key for looking up a configuration value for @c CURLOPT_CAPATH
static const std::string CAPATH_CONFIG_KEY = "CURLOPT_CAPATH";

/// Key for looking up a configuration value for verifying hosts and peers.
static const std::string VERIFY_HOSTS_AND_PEERS_CONFIG_KEY = "verifyHostsAndPeers";

/**
 * Set an @c option on a @c libcurl handle to @c value with stringification of @c option name and @c value for logging.
 *
 * @param handle The CURL handle to set the option on.
 * @param option The option to set.
 * @param value The new value for the specified option.
 */
#define SETOPT(handle, option, value) setopt(handle, option, value, #option, #value)

/**
 * Set an @c option on a @c libcurl handle to @c value.
 *
 * @tparam ValueType The type of value being set
 * @param handle The CURL handle to set the option on.
 * @param option The option to set.
 * @param value The new value for the specified option.
 * @param optionName The name of the option being set.
 * @param valueAsString String representation of the value being set.
 * @return Whether the option was set.
 */
template <typename ValueType>
static bool setopt(
    CURL* handle,
    CURLoption option,
    ValueType value,
    const char* optionName,
    const char* valueAsString) {
    auto result = curl_easy_setopt(handle, option, value);
    if (result != CURLE_OK) {
        ACSDK_ERROR(LX("curl_easy_setoptFailed")
                        .d("option", optionName)
                        .sensitive("value", valueAsString)
                        .d("result", result)
                        .d("error", curl_easy_strerror(result)));
        return false;
    }
    return true;
}

bool prepareForTLS(CURL* handle) {
    if (!handle) {
        ACSDK_ERROR(LX("prepareForTLSFailed").d("reason", "nullHandle"));
        return false;
    }
    if (!(SETOPT(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2) &&
          SETOPT(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0) &&
          SETOPT(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL) && SETOPT(handle, CURLOPT_SSL_VERIFYPEER, 1L) &&
          SETOPT(handle, CURLOPT_SSL_VERIFYHOST, 2L))) {
        return false;
    }

    auto config = configuration::ConfigurationNode::getRoot()[LIBCURLUTILS_CONFIG_KEY];

    std::string caPath;
    if (config.getString(CAPATH_CONFIG_KEY, &caPath) &&
        !setopt(handle, CURLOPT_CAPATH, caPath.c_str(), "CURLOPT_CAPATH", caPath.c_str())) {
        return false;
    }

// Only allow disabling the verification of hosts and peers in debug configurations.
#ifdef DEBUG
    bool verifyHostsAndPeers = true;
    if (config.getBool(VERIFY_HOSTS_AND_PEERS_CONFIG_KEY, &verifyHostsAndPeers) && !verifyHostsAndPeers) {
        if (!(SETOPT(handle, CURLOPT_SSL_VERIFYPEER, 0L) && SETOPT(handle, CURLOPT_SSL_VERIFYHOST, 0L))) {
            return false;
        }
        ACSDK_WARN(LX("verificationOfHostsAndPeersDisabled"));
    }
#endif

    return true;
}

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
