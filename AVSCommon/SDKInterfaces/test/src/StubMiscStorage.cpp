/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/Storage/StubMiscStorage.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace storage {
namespace test {

bool StubMiscStorage::createDatabase() {
    return true;
}

bool StubMiscStorage::open() {
    m_isOpened = true;
    return true;
}

void StubMiscStorage::close() {
    m_isOpened = false;
}

bool StubMiscStorage::createTable(
    const std::string& componentName,
    const std::string& tableName,
    MiscStorageInterface::KeyType keyType,
    MiscStorageInterface::ValueType valueType) {
    std::string key = componentName + ":" + tableName;
    m_tables.insert(key);
    return true;
}

bool StubMiscStorage::clearTable(const std::string& componentName, const std::string& tableName) {
    std::string keyPrefix = componentName + ":" + tableName + ":";
    auto it = m_storage.begin();
    while (it != m_storage.end()) {
        const std::string& key = it->first;
        size_t pos = key.find(keyPrefix);
        if (std::string::npos != pos) {
            it = m_storage.erase(it);
        } else {
            ++it;
        }
    }
    return true;
}

bool StubMiscStorage::deleteTable(const std::string& componentName, const std::string& tableName) {
    std::string key = componentName + ":" + tableName;
    m_tables.erase(key);
    return clearTable(componentName, tableName);
}

bool StubMiscStorage::get(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    std::string* value) {
    std::string keyStr = componentName + ":" + tableName + ":" + key;
    auto it = m_storage.find(keyStr);
    if (m_storage.end() == it) {
        return false;
    }
    *value = it->second;
    return true;
}

bool StubMiscStorage::add(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    return put(componentName, tableName, key, value);
}

bool StubMiscStorage::update(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    return put(componentName, tableName, key, value);
}

bool StubMiscStorage::put(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    std::string keyStr = componentName + ":" + tableName + ":" + key;
    m_storage[keyStr] = value;
    return true;
}

bool StubMiscStorage::remove(const std::string& componentName, const std::string& tableName, const std::string& key) {
    std::string keyStr = componentName + ":" + tableName + ":" + key;
    m_storage.erase(keyStr);
    return true;
}

bool StubMiscStorage::tableEntryExists(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    bool* tableEntryExistsValue) {
    std::string keyStr = componentName + ":" + tableName + ":" + key;
    auto it = m_storage.find(keyStr);
    *tableEntryExistsValue = m_storage.end() != it;
    return true;
}

bool StubMiscStorage::tableExists(
    const std::string& componentName,
    const std::string& tableName,
    bool* tableExistsValue) {
    std::string key = componentName + ":" + tableName;
    bool exists = m_tables.end() != m_tables.find(key);
    *tableExistsValue = exists;
    return true;
}

bool StubMiscStorage::load(
    const std::string& componentName,
    const std::string& tableName,
    std::unordered_map<std::string, std::string>* valueContainer) {
    std::string keyStr = componentName + ":" + tableName + ":";
    size_t keyLen = keyStr.length();
    for (const auto& it : m_storage) {
        const std::string& key = it.first;
        if (key.substr(0, keyLen) == keyStr) {
            std::string targetKey = key.substr(keyLen);
            valueContainer->insert(std::pair<std::string, std::string>(targetKey, it.second));
        }
    }
    *valueContainer = m_storage;
    return true;
}

std::shared_ptr<StubMiscStorage> StubMiscStorage::create() {
    return std::shared_ptr<StubMiscStorage>(new StubMiscStorage());
}

StubMiscStorage::StubMiscStorage() : m_isOpened{false} {
}

bool StubMiscStorage::isOpened() {
    return m_isOpened;
}

}  // namespace test
}  // namespace storage
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK
