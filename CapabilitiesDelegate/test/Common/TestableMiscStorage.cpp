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

#include "TestableMiscStorage.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

/// Separator between keys
static const std::string DB_KEY_SEPARATOR = ",";
/// Component key
static const std::string DB_KEY_COMPONENT = "component:";
/// Table key
static const std::string DB_KEY_TABLE = "table:";

/**
 * Gets the key that is actually used to store values in the DB.
 *
 * @param componentName The interface type.
 * @param tableName The interface name.
 * @param key The interface version.
 * @return interfaceConfig The interface config as a json string.
 */
static std::string getInternalKey(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key);

bool TestMiscStorage::createDatabase() {
    return true;
}

bool TestMiscStorage::open() {
    return true;
}

bool TestMiscStorage::isOpened() {
    return true;
}

void TestMiscStorage::close() {
}

bool TestMiscStorage::createTable(
    const std::string& componentName,
    const std::string& tableName,
    KeyType keyType,
    ValueType valueType) {
    return true;
}

bool TestMiscStorage::deleteTable(const std::string& componentName, const std::string& tableName) {
    return clearTable(componentName, tableName);
}

bool TestMiscStorage::clearTable(const std::string& componentName, const std::string& tableName) {
    std::string keyPrefix = DB_KEY_COMPONENT + componentName + DB_KEY_SEPARATOR + DB_KEY_TABLE + tableName;
    size_t keyPrefixLength = keyPrefix.length();

    for (auto iter = m_miscDB_stringKeyValue.begin(); iter != m_miscDB_stringKeyValue.end();) {
        std::string mapEntryKey = iter->first;
        // Increment iterator before erase to get next element.
        iter++;
        if (mapEntryKey.substr(0, keyPrefixLength) == keyPrefix) {
            m_miscDB_stringKeyValue.erase(mapEntryKey);
        }
    }

    return true;
}

bool TestMiscStorage::add(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    return put(componentName, tableName, key, value);
}

bool TestMiscStorage::update(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    return put(componentName, tableName, key, value);
}

bool TestMiscStorage::put(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    const std::string& value) {
    m_miscDB_stringKeyValue[getInternalKey(componentName, tableName, key)] = value;
    return true;
}

bool TestMiscStorage::remove(const std::string& componentName, const std::string& tableName, const std::string& key) {
    m_miscDB_stringKeyValue.erase(getInternalKey(componentName, tableName, key));
    return true;
}

bool TestMiscStorage::tableEntryExists(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    bool* tableEntryExistsValue) {
    if (!tableEntryExistsValue) {
        return false;
    }

    auto mapIterator = m_miscDB_stringKeyValue.find(getInternalKey(componentName, tableName, key));
    if (mapIterator == m_miscDB_stringKeyValue.end()) {
        *tableEntryExistsValue = false;
    } else {
        *tableEntryExistsValue = true;
    }
    return true;
}

bool TestMiscStorage::tableExists(
    const std::string& componentName,
    const std::string& tableName,
    bool* tableExistsValue) {
    if (!tableExistsValue) {
        return false;
    }

    *tableExistsValue = true;
    return true;
}

bool TestMiscStorage::load(
    const std::string& componentName,
    const std::string& tableName,
    std::unordered_map<std::string, std::string>* valueContainer) {
    if (!valueContainer) {
        return false;
    }

    std::string keyPrefix = DB_KEY_COMPONENT + componentName + DB_KEY_SEPARATOR + DB_KEY_TABLE + tableName;
    size_t keyPrefixLength = keyPrefix.length();

    for (auto mapEntry : m_miscDB_stringKeyValue) {
        std::string mapEntryKey = mapEntry.first;
        if (mapEntryKey.substr(0, keyPrefixLength) == keyPrefix) {
            valueContainer->insert(mapEntry);
        }
    }

    return true;
}

bool TestMiscStorage::get(
    const std::string& componentName,
    const std::string& tableName,
    const std::string& key,
    std::string* value) {
    auto mapIterator = m_miscDB_stringKeyValue.find(getInternalKey(componentName, tableName, key));
    if (mapIterator == m_miscDB_stringKeyValue.end()) {
        return false;
    }
    *value = mapIterator->second;
    return true;
}

bool TestMiscStorage::dataExists(const std::string& componentName, const std::string& tableName) {
    std::string keyPrefix = DB_KEY_COMPONENT + componentName + DB_KEY_SEPARATOR + DB_KEY_TABLE + tableName;
    size_t keyPrefixLength = keyPrefix.length();

    for (auto mapEntry : m_miscDB_stringKeyValue) {
        std::string mapEntryKey = mapEntry.first;
        if (mapEntryKey.substr(0, keyPrefixLength) == keyPrefix) {
            return true;
        }
    }
    return false;
}

std::string getInternalKey(const std::string& componentName, const std::string& tableName, const std::string& key) {
    return DB_KEY_COMPONENT + componentName + DB_KEY_SEPARATOR + DB_KEY_TABLE + tableName + DB_KEY_SEPARATOR + key;
}

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK
