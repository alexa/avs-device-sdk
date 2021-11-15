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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_STORAGE_MOCKMISCSTORAGE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_STORAGE_MOCKMISCSTORAGE_H_

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
 * Mock for @c MiscStorageInterface.
 */
class MockMiscStorage : public avsCommon::sdkInterfaces::storage::MiscStorageInterface {
public:
    /// @name MiscStorageInterface functions
    /// @{
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(isOpened, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD4(
        createTable,
        bool(const std::string& componentName, const std::string& tableName, KeyType keyType, ValueType valueType));
    MOCK_METHOD2(clearTable, bool(const std::string& componentName, const std::string& tableName));
    MOCK_METHOD2(deleteTable, bool(const std::string& componentName, const std::string& tableName));
    MOCK_METHOD4(
        get,
        bool(
            const std::string& componentName,
            const std::string& tableName,
            const std::string& key,
            std::string* value));
    MOCK_METHOD4(
        add,
        bool(
            const std::string& componentName,
            const std::string& tableName,
            const std::string& key,
            const std::string& value));
    MOCK_METHOD4(
        update,
        bool(
            const std::string& componentName,
            const std::string& tableName,
            const std::string& key,
            const std::string& value));
    MOCK_METHOD4(
        put,
        bool(
            const std::string& componentName,
            const std::string& tableName,
            const std::string& key,
            const std::string& value));
    MOCK_METHOD3(remove, bool(const std::string& componentName, const std::string& tableName, const std::string& key));
    MOCK_METHOD4(
        tableEntryExists,
        bool(
            const std::string& componentName,
            const std::string& tableName,
            const std::string& key,
            bool* tableEntryExistsValue));
    MOCK_METHOD3(
        tableExists,
        bool(const std::string& componentName, const std::string& tableName, bool* tableExistsValue));
    MOCK_METHOD3(
        load,
        bool(
            const std::string& componentName,
            const std::string& tableName,
            std::unordered_map<std::string, std::string>* valueContainer));
    ///@}
};

}  // namespace test
}  // namespace storage
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_STORAGE_MOCKMISCSTORAGE_H_
