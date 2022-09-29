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

#include <acsdk/PropertiesInterfaces/test/StubProperties.h>

namespace alexaClientSDK {
namespace propertiesInterfaces {
namespace test {

/// @private
static constexpr char TYPE_BIN = 'b';
/// @private
static constexpr char TYPE_STR = 's';

std::string StubProperties::createFullyQualifiedName(const std::string& keyName) noexcept {
    return createKeyPrefix() + keyName;
}

std::string StubProperties::createKeyPrefix() noexcept {
    return m_configUri + "/";
}

StubProperties::StubProperties(
    const std::shared_ptr<StubPropertiesFactory>& owner,
    const std::string& configUri) noexcept :
        m_owner{owner},
        m_configUri{configUri} {
}

bool StubProperties::getString(const std::string& key, std::string& value) noexcept {
    std::string keyStr = createFullyQualifiedName(key);

    auto it = m_owner->m_storage.find(keyStr);
    if (m_owner->m_storage.end() == it) {
        return false;
    }
    if (TYPE_STR != it->second.first) {
        return false;
    }
    value.assign(it->second.second.data(), it->second.second.data() + it->second.second.size());
    return true;
}

bool StubProperties::putString(const std::string& key, const std::string& value) noexcept {
    std::string keyStr = createFullyQualifiedName(key);
    Bytes bin(value.c_str(), value.c_str() + value.size());
    m_owner->m_storage[keyStr] = std::pair<char, Bytes>(TYPE_STR, bin);
    return true;
}

bool StubProperties::getBytes(const std::string& key, Bytes& value) noexcept {
    std::string keyStr = createFullyQualifiedName(key);

    auto it = m_owner->m_storage.find(keyStr);
    if (m_owner->m_storage.end() == it) {
        return false;
    }
    if (TYPE_BIN != it->second.first) {
        return false;
    }
    value = it->second.second;
    return true;
}

bool StubProperties::putBytes(const std::string& key, const Bytes& value) noexcept {
    std::string keyStr = createFullyQualifiedName(key);
    m_owner->m_storage[keyStr] = std::pair<char, Bytes>(TYPE_BIN, value);
    return true;
}

bool StubProperties::remove(const std::string& key) noexcept {
    std::string keyStr = createFullyQualifiedName(key);
    m_owner->m_storage.erase(keyStr);
    return true;
}

bool StubProperties::getKeys(std::unordered_set<std::string>& keys) noexcept {
    std::string keyPrefix = createKeyPrefix();
    size_t keyLen = keyPrefix.length();
    for (const auto& it : m_owner->m_storage) {
        const std::string& key = it.first;
        if (key.compare(0, keyLen, keyPrefix) == 0) {
            std::string targetKey = key.substr(keyLen);
            keys.insert(targetKey);
        }
    }
    return true;
}

bool StubProperties::clear() noexcept {
    std::string keyPrefix = createKeyPrefix();
    size_t keyLen = keyPrefix.length();
    for (auto it = m_owner->m_storage.begin(); it != m_owner->m_storage.end();) {
        const std::string& key = it->first;
        if (key.compare(0, keyLen, keyPrefix) == 0) {
            it = m_owner->m_storage.erase(it);
        } else {
            it++;
        }
    }
    return true;
}

}  // namespace test
}  // namespace propertiesInterfaces
}  // namespace alexaClientSDK
