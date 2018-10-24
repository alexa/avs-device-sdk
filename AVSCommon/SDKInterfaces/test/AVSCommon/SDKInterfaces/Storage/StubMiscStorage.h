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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_STORAGE_STUBMISCSTORAGE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_STORAGE_STUBMISCSTORAGE_H_

#include <gmock/gmock.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace storage {
namespace test {

/**
 * In memory implementation for @c MiscStorageInterface. This class is not thread safe.
 */
class StubMiscStorage : public avsCommon::sdkInterfaces::storage::MiscStorageInterface {
public:
    static std::shared_ptr<StubMiscStorage> create();

    /// @name MiscStorageInterface functions
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

    bool get(const std::string& componentName, const std::string& tableName, const std::string& key, std::string* value)
        override;

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
    ///@}

private:
    StubMiscStorage();

    /// Container to keep stored values. The format of the key is "componentName:tableName:key".
    std::unordered_map<std::string, std::string> m_storage;
    /// A collection of table prefixes to track if table exists.
    std::unordered_set<std::string> m_tables;
    /// Flag indicating if database is opened.
    bool m_isOpened;
};

}  // namespace test
}  // namespace storage
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_STORAGE_STUBMISCSTORAGE_H_
