/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ACSDK_PROPERTIES_PRIVATE_LOGGING_H_
#define ACSDK_PROPERTIES_PRIVATE_LOGGING_H_

#include <AVSCommon/Utils/Logger/Logger.h>

/**
 * @brief Create a LogEntry.
 *
 * Create a LogEntry using TAG constant and the specified event string.
 *
 * @param[in] event The event string for this @c LogEntry.
 * @private
 * @ingroup properties
 */
#define LX(event) ::alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * @brief Create a LogEntry for configuration event.
 *
 * @param[in] event The event string for this @c LogEntry.
 * @param[in] configUri Configuration URI.
 * @private
 * @ingroup properties
 */
#define LX_CFG(event, configUri) LX(event).d(::alexaClientSDK::properties::CONFIG_URI, configUri)

/**
 * @brief Create a LogEntry for configuration event.
 *
 * @param[in] event The event string for this @c LogEntry.
 * @param[in] configUri Configuration URI.
 * @param[in] key Key name.
 * @private
 * @ingroup properties
 */
#define LX_CFG_KEY(event, configUri, key) LX_CFG(event, configUri).d(::alexaClientSDK::properties::KEY, key)

/**
 * @brief Macro to help compiler happy when variable is unused.
 *
 * When variables are passed for logging events, compiler complains when logging is disabled (as variables become
 * unused. This macro marks variable as used to suppress build errors.
 *
 * @param[in] var Variable name.
 *
 * @private
 * @ingroup properties
 */
#define ACSDK_UNUSED_VARIABLE(var) (void)var

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

template <>
LogEntry& LogEntry::d(const char* key, const std::vector<unsigned char>& value);

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace alexaClientSDK {
namespace properties {

/// String to identify config URI.
/// @private
extern const std::string CONFIG_URI;

/// String to identify key.
/// @private
extern const std::string KEY;

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_LOGGING_H_
