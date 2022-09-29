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

#include <acsdk/Properties/private/MiscStoragePropertiesFactory.h>
#include <AVSCommon/SDKInterfaces/Storage/MockMiscStorage.h>

namespace alexaClientSDK {
namespace properties {
namespace test {

using namespace ::testing;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage;
using namespace ::alexaClientSDK::avsCommon::sdkInterfaces::storage::test;

/// Test constructor.
TEST(MiscStoragePropertiesFactoryTest, test_createOpened) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();
    EXPECT_CALL(*mockMiscStorage, isOpened()).WillOnce(Return(true));
    EXPECT_CALL(*mockMiscStorage, createDatabase()).Times(0);
    EXPECT_CALL(*mockMiscStorage, open()).Times(0);

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());

    ASSERT_NE(nullptr, factory);
}

TEST(MiscStoragePropertiesFactoryTest, test_createClosed) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();
    EXPECT_CALL(*mockMiscStorage, isOpened()).WillOnce(Return(false));
    EXPECT_CALL(*mockMiscStorage, open()).WillOnce(Return(true));
    EXPECT_CALL(*mockMiscStorage, createDatabase()).Times(0);

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());

    ASSERT_NE(nullptr, factory);
}

TEST(MiscStoragePropertiesFactoryTest, test_createDatabase) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();
    EXPECT_CALL(*mockMiscStorage, isOpened()).WillOnce(Return(false));
    EXPECT_CALL(*mockMiscStorage, open()).WillOnce(Return(false));
    EXPECT_CALL(*mockMiscStorage, createDatabase()).WillOnce(Return(true));

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());

    ASSERT_NE(nullptr, factory);
}

TEST(MiscStoragePropertiesFactoryTest, test_createWhenStorageClosedAndCreateFails) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();
    EXPECT_CALL(*mockMiscStorage, isOpened()).WillOnce(Return(false));
    EXPECT_CALL(*mockMiscStorage, open()).WillOnce(Return(false));
    EXPECT_CALL(*mockMiscStorage, createDatabase()).WillOnce(Return(false));

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());

    ASSERT_EQ(nullptr, factory);
}

TEST(MiscStoragePropertiesFactoryTest, test_createNullArgs) {
    auto factory = MiscStoragePropertiesFactory::create(nullptr, SimpleMiscStorageUriMapper::create());

    EXPECT_EQ(nullptr, factory);

    auto mockMiscStorage = std::make_shared<MockMiscStorage>();
    factory = MiscStoragePropertiesFactory::create(mockMiscStorage, nullptr);
    ASSERT_EQ(nullptr, factory);
}

TEST(MiscStoragePropertiesFactoryTest, test_createProperties) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, isOpened()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string& comp, const std::string& name, bool* res) -> bool {
            *res = true;
            return true;
        }));

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());
    ASSERT_NE(nullptr, factory);

    auto properties = factory->getProperties("component/namespace");
    ASSERT_NE(nullptr, properties);
}

TEST(MiscStoragePropertiesFactoryTest, test_createPropertiesCreatesTable) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, isOpened()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string& comp, const std::string& name, bool* res) -> bool {
            *res = false;
            return true;
        }));
    EXPECT_CALL(*mockMiscStorage, createTable(Eq("component"), Eq("namespace"), _, _)).WillOnce(Return(true));

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());
    ASSERT_NE(nullptr, factory);

    auto properties = factory->getProperties("component/namespace");
    ASSERT_NE(nullptr, properties);
}

TEST(MiscStoragePropertiesFactoryTest, test_createPropertiesFailsToCreateTable) {
    auto mockMiscStorage = std::make_shared<MockMiscStorage>();

    EXPECT_CALL(*mockMiscStorage, isOpened()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockMiscStorage, tableExists(Eq("component"), Eq("namespace"), _))
        .WillOnce(Invoke([](const std::string& comp, const std::string& name, bool* res) -> bool {
            *res = false;
            return true;
        }));
    EXPECT_CALL(*mockMiscStorage, createTable(Eq("component"), Eq("namespace"), _, _)).WillOnce(Return(false));

    auto factory = MiscStoragePropertiesFactory::create(mockMiscStorage, SimpleMiscStorageUriMapper::create());
    ASSERT_NE(nullptr, factory);

    auto properties = factory->getProperties("component/namespace");
    ASSERT_EQ(nullptr, properties);
}

}  // namespace test
}  // namespace properties
}  // namespace alexaClientSDK
