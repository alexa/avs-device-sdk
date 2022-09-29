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

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <acsdk/Properties/private/MiscStorageProperties.h>
#include <AVSCommon/SDKInterfaces/Storage/MockMiscStorage.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace properties {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage::test;

/// String to identify log entries originating from this file.
/// @private
#define TAG "MiscStoragePropertiesTest"

/// Test that the constructor.
TEST(MiscStoragePropertiesTest, testCreatePropertiesTableExists) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");

    ASSERT_NE(nullptr, props);
}

TEST(MiscStoragePropertiesTest, testCreatePropertiesCreateTable) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = false;
            return true;
        }));
    EXPECT_CALL(
        *mockMiscStorage,
        createTable(
            Eq("component"),
            Eq("namespace"),
            Eq(MiscStorageInterface::KeyType::STRING_KEY),
            Eq(MiscStorageInterface::ValueType::STRING_VALUE)))
        .WillOnce(Invoke(
            [](const std::string&, const std::string&, MiscStorageInterface::KeyType, MiscStorageInterface::ValueType) {
                return true;
            }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");

    ASSERT_NE(nullptr, props);
}

TEST(MiscStoragePropertiesTest, testCreatePropertiesCreateTableFailed) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = false;
            return true;
        }));
    EXPECT_CALL(
        *mockMiscStorage,
        createTable(
            Eq("component"),
            Eq("namespace"),
            Eq(MiscStorageInterface::KeyType::STRING_KEY),
            Eq(MiscStorageInterface::ValueType::STRING_VALUE)))
        .WillOnce(Invoke(
            [](const std::string&, const std::string&, MiscStorageInterface::KeyType, MiscStorageInterface::ValueType) {
                return false;
            }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");

    ASSERT_EQ(nullptr, props);
}

TEST(MiscStoragePropertiesTest, testCreatePropertiesTableExistsFailed) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) { return false; }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");

    ASSERT_EQ(nullptr, props);
}

TEST(MiscStoragePropertiesTest, testPutString) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, put(Eq("component"), Eq("namespace"), Eq("key"), Eq("value"))).WillOnce(Return(true));
    ASSERT_EQ(true, props->putString("key", "value"));
}

TEST(MiscStoragePropertiesTest, testPutBytes) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, put(Eq("component"), Eq("namespace"), Eq("key"), Eq("AAEC"))).WillOnce(Return(true));
    ASSERT_TRUE(props->putBytes("key", MiscStorageProperties::Bytes{0, 1, 2}));
}

TEST(MiscStoragePropertiesTest, testPutFailed) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, put(Eq("component"), Eq("namespace"), Eq("key"), Eq("value")))
        .WillOnce(Return(false));
    ASSERT_FALSE(props->putString("key", "value"));
}

TEST(MiscStoragePropertiesTest, testGetString) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, get(Eq("component"), Eq("namespace"), Eq("key"), _))
        .WillOnce(Invoke([](const std::string&, const std::string&, const std::string&, std::string* res) -> bool {
            *res = "value";
            return true;
        }));
    std::string value;
    ASSERT_TRUE(props->getString("key", value));
    ASSERT_EQ("value", value);
}

TEST(MiscStoragePropertiesTest, testGetBinary) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, get(Eq("component"), Eq("namespace"), Eq("key"), _))
        .WillOnce(Invoke([](const std::string&, const std::string&, const std::string&, std::string* res) -> bool {
            *res = "AAEC";
            return true;
        }));
    MiscStorageProperties::Bytes value;
    ASSERT_TRUE(props->getBytes("key", value));
    ASSERT_EQ((MiscStorageProperties::Bytes{0, 1, 2}), value);
}

TEST(MiscStoragePropertiesTest, testGetFailed) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, get(Eq("component"), Eq("namespace"), Eq("key"), _)).WillOnce(Return(false));
    std::string tmp;
    ASSERT_FALSE(props->getString("key", tmp));
}

TEST(MiscStoragePropertiesTest, testRemove) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, remove(Eq("component"), Eq("namespace"), Eq("key"))).WillOnce(Return(true));
    ASSERT_TRUE(props->remove("key"));
}

TEST(MiscStoragePropertiesTest, testClear) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, clearTable(Eq("component"), Eq("namespace"))).WillOnce(Return(true));
    ASSERT_TRUE(props->clear());
}

TEST(MiscStoragePropertiesTest, testClearFailedClearTable) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, tableExists(_, _, _))
        .WillOnce(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    auto props = MiscStorageProperties::create(mockMiscStorage, "component/namespace", "component", "namespace");
    ASSERT_NE(nullptr, props);

    EXPECT_CALL(*mockMiscStorage, clearTable(Eq("component"), Eq("namespace"))).WillOnce(Return(false));
    ASSERT_FALSE(props->clear());
}

}  // namespace test
}  // namespace properties
}  // namespace alexaClientSDK
