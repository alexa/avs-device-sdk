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

#include <unordered_map>

#include <acsdk/Properties/private/Logging.h>
#include <acsdk/CodecUtils/Hex.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

template <>
LogEntry& LogEntry::d(const char* key, const std::vector<unsigned char>& value) {
    prefixKeyValuePair();
    std::string hexValue;
    alexaClientSDK::codecUtils::encodeHex(value, hexValue);
    m_stream << key << KEY_VALUE_SEPARATOR << hexValue;
    return *this;
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace alexaClientSDK {
namespace properties {

const std::string CONFIG_URI{"configUri"};
const std::string KEY{"key"};

}  // namespace properties
}  // namespace alexaClientSDK
