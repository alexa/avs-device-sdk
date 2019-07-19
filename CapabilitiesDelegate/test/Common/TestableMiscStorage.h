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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEMISCSTORAGE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEMISCSTORAGE_H_

#include <string>
#include <unordered_map>

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {
namespace test {

/**
 * A test MiscStorage instance that will let you simulate working with the actual Misc DB.
 */
class TestMiscStorage : public avsCommon::sdkInterfaces::storage::MiscStorageInterface {
public:
    /**
     * Constructor
     */
    TestMiscStorage() = default;

    /// @name MiscStorageInterface method overrides.
    /// @{
    bool createDatabase() override;

    bool open() override;

    bool isOpened() override;

    void close() override;

    bool createTable(
        const std::string& componentName,
        const std::string& tableName,
        KeyType keyType,
        ValueType valueType) override;

    bool clearTable(const std::string& componentName, const std::string& tableName) override;

    bool deleteTable(const std::string& componentName, const std::string& tableName) override;

    bool add(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) override;

    bool update(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) override;

    bool put(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        const std::string& value) override;

    bool remove(const std::string& componentName, const std::string& tableName, const std::string& key) override;

    bool tableEntryExists(
        const std::string& componentName,
        const std::string& tableName,
        const std::string& key,
        bool* tableEntryExistsValue) override;

    bool tableExists(const std::string& componentName, const std::string& tableName, bool* tableExistsValue) override;

    bool load(
        const std::string& componentName,
        const std::string& tableName,
        std::unordered_map<std::string, std::string>* valueContainer) override;

    bool get(const std::string& componentName, const std::string& tableName, const std::string& key, std::string* value)
        override;
    /// @}

    /**
     * Returns whether or not data exists for a table in the DB.
     *
     * @param componentName The interface type.
     * @param tableName The interface name.
     * @return true if data exists for the table, else false.
     */
    bool dataExists(const std::string& componentName, const std::string& tableName);

private:
    std::unordered_map<std::string, std::string> m_miscDB_stringKeyValue;
};

}  // namespace test
}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_TEST_COMMON_TESTABLEMISCSTORAGE_H_ */
