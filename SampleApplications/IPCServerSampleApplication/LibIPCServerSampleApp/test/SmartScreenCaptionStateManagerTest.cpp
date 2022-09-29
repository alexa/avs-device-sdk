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

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-generated-function-mockers.h>
#include "IPCServerSampleApp/SmartScreenCaptionStateManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace test {

using namespace ::testing;

static const std::string componentName = "IPCServerSampleApp";
static const std::string tableName = "Settings";
static const std::string captionsKey = "CaptionsEnabled";
static const std::string captionsEnabledString = "CAPTIONS_ENABLED";
static const std::string captionsDisabledString = "CAPTIONS_DISABLED";

class MockMiscStorage : public avsCommon::sdkInterfaces::storage::MiscStorageInterface {
public:
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
};

class SmartScreenCaptionStateManagerTest : public ::testing::Test {
public:
    void SetUp() override;

protected:
    std::shared_ptr<NiceMock<MockMiscStorage>> m_mockMiscStorage;
    std::shared_ptr<SmartScreenCaptionStateManager> captionManager;

    void expectTableToExist();
};

void SmartScreenCaptionStateManagerTest::SetUp() {
    m_mockMiscStorage = std::make_shared<NiceMock<MockMiscStorage>>();
}

void SmartScreenCaptionStateManagerTest::expectTableToExist() {
    ON_CALL(*m_mockMiscStorage, tableExists(componentName, tableName, NotNull()))
        .WillByDefault(DoAll(SetArgPointee<2>(true), Return(true)));
    captionManager = std::make_shared<SmartScreenCaptionStateManager>(m_mockMiscStorage);
}

/**
 * Tests getCaptions on true value
 */
TEST_F(SmartScreenCaptionStateManagerTest, test_getCaptionSettingWhenDatabaseReturnsTrue) {
    expectTableToExist();

    EXPECT_CALL(*m_mockMiscStorage, get(componentName, tableName, captionsKey, NotNull()))
        .Times(Exactly(1))
        .WillOnce(DoAll(SetArgPointee<3>(captionsEnabledString), Return(true)));

    ASSERT_EQ(captionManager->areCaptionsEnabled(), true);
}

/**
 * Tests getCaptions on false value
 */
TEST_F(SmartScreenCaptionStateManagerTest, test_getCaptionSettingWhenDataBaseReturnsFalse) {
    expectTableToExist();

    EXPECT_CALL(*m_mockMiscStorage, get(componentName, tableName, captionsKey, NotNull()))
        .Times(Exactly(1))
        .WillOnce(DoAll(SetArgPointee<3>(captionsDisabledString), Return(true)));

    ASSERT_EQ(captionManager->areCaptionsEnabled(), false);
}

/**
 * Tests getCaptions on database error
 */
TEST_F(SmartScreenCaptionStateManagerTest, test_getCaptionSettingWhenDataBaseStorageFailure) {
    expectTableToExist();

    EXPECT_CALL(*m_mockMiscStorage, get(componentName, tableName, captionsKey, NotNull()))
        .Times(Exactly(1))
        .WillOnce(Return(false));

    ASSERT_EQ(captionManager->areCaptionsEnabled(), false);
}

/**
 * Tests set captions enabled
 */
TEST_F(SmartScreenCaptionStateManagerTest, test_setCaptionsEnabled) {
    expectTableToExist();

    EXPECT_CALL(*m_mockMiscStorage, put(componentName, tableName, captionsKey, captionsEnabledString))
        .Times(Exactly(1))
        .WillOnce(Return(true));

    captionManager->setCaptionsState(true);
}

/**
 * Tests set captions disabled
 */
TEST_F(SmartScreenCaptionStateManagerTest, test_setCaptionsDisabled) {
    expectTableToExist();

    EXPECT_CALL(*m_mockMiscStorage, put(componentName, tableName, captionsKey, captionsDisabledString))
        .Times(Exactly(1))
        .WillOnce(Return(true));

    captionManager->setCaptionsState(false);
}

/**
 * Create table if it does not exist
 */
TEST_F(SmartScreenCaptionStateManagerTest, test_createTableInDatabaseIfItDoesNotExist) {
    EXPECT_CALL(*m_mockMiscStorage, tableExists(componentName, tableName, NotNull()))
        .Times(Exactly(1))
        .WillOnce(DoAll(SetArgPointee<2>(false), Return(true)));

    EXPECT_CALL(
        *m_mockMiscStorage,
        createTable(
            componentName,
            tableName,
            avsCommon::sdkInterfaces::storage::MiscStorageInterface::KeyType::STRING_KEY,
            avsCommon::sdkInterfaces::storage::MiscStorageInterface::ValueType::STRING_VALUE))
        .Times(Exactly(1))
        .WillOnce(Return(true));

    captionManager = std::make_shared<SmartScreenCaptionStateManager>(m_mockMiscStorage);
}

}  // namespace test
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK