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

#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <acsdk/SpeakerManager/private/SpeakerManagerMiscStorage.h>

namespace alexaClientSDK {
namespace speakerManager {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::avs::speakerConstants;
using namespace avsCommon::sdkInterfaces;
using namespace ::testing;

/// @addtogroup Test_acsdkSpeakerManager
/// @{

class MockMiscStorage : public storage::MiscStorageInterface {
public:
    MOCK_METHOD0(createDatabase, bool());
    MOCK_METHOD0(open, bool());
    MOCK_METHOD0(isOpened, bool());
    MOCK_METHOD0(close, void());
    MOCK_METHOD4(createTable, bool(const std::string&, const std::string&, KeyType, ValueType));
    MOCK_METHOD2(deleteTable, bool(const std::string&, const std::string&));
    MOCK_METHOD2(clearTable, bool(const std::string&, const std::string&));
    MOCK_METHOD4(get, bool(const std::string&, const std::string&, const std::string&, std::string*));
    MOCK_METHOD4(add, bool(const std::string&, const std::string&, const std::string&, const std::string&));
    MOCK_METHOD4(update, bool(const std::string&, const std::string&, const std::string&, const std::string&));
    MOCK_METHOD4(put, bool(const std::string&, const std::string&, const std::string&, const std::string&));
    MOCK_METHOD3(remove, bool(const std::string&, const std::string&, const std::string&));
    MOCK_METHOD4(tableEntryExists, bool(const std::string&, const std::string&, const std::string&, bool*));
    MOCK_METHOD3(tableExists, bool(const std::string&, const std::string&, bool*));
    MOCK_METHOD3(
        load,
        bool(const std::string&, const std::string&, std::unordered_map<std::string, std::string>* valueContainer));
};

/// The @c MessageId identifer.
static const std::string JSON_PAYLOAD =
    ""
    "{"
    "  \"speakerChannelState\": {"
    "    \"channelVolume\": 10,"
    "    \"channelMuteStatus\": false"
    "  },"
    "  \"alertsChannelState\": {"
    "    \"channelVolume\": 15,"
    "    \"channelMuteStatus\": true"
    "  }"
    "}";

/// @brief Test fixture for SpeakerManagerMiscStorage.
class SpeakerManagerMiscStorageTest : public ::testing::TestWithParam<std::vector<ChannelVolumeInterface::Type>> {
public:
    /// SetUp before each test.
    void SetUp() override;

    /// TearDown after each test.
    void TearDown() override;

protected:
    // Upstream interface mock
    std::shared_ptr<NiceMock<MockMiscStorage>> m_stubMiscStorage;
};

/// @}

void SpeakerManagerMiscStorageTest::SetUp() {
    m_stubMiscStorage = std::make_shared<NiceMock<MockMiscStorage>>();
}

void SpeakerManagerMiscStorageTest::TearDown() {
    m_stubMiscStorage.reset();
}

TEST_F(SpeakerManagerMiscStorageTest, test_nullMiscStorage) {
    ASSERT_EQ(nullptr, SpeakerManagerMiscStorage::create(nullptr));
}

TEST_F(SpeakerManagerMiscStorageTest, test_FailedOpen) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(false));
    ON_CALL(*m_stubMiscStorage, open()).WillByDefault(Return(false));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, open()).Times(1);

    ASSERT_EQ(nullptr, SpeakerManagerMiscStorage::create(m_stubMiscStorage));
}

TEST_F(SpeakerManagerMiscStorageTest, test_OpenAndFailedCheckTableStatus) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _)).WillByDefault(Return(false));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, open()).Times(0);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, createTable(_, _, _, _)).Times(0);

    ASSERT_EQ(nullptr, SpeakerManagerMiscStorage::create(m_stubMiscStorage));
}

TEST_F(SpeakerManagerMiscStorageTest, test_OpenAndFailedCreateTable) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = false;
            return true;
        }));
    ON_CALL(*m_stubMiscStorage, createTable(_, _, _, _)).WillByDefault(Return(false));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, open()).Times(0);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, createTable(_, _, _, _)).Times(1);

    ASSERT_EQ(nullptr, SpeakerManagerMiscStorage::create(m_stubMiscStorage));
}

TEST_F(SpeakerManagerMiscStorageTest, test_CreateTable) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = false;
            return true;
        }));
    ON_CALL(*m_stubMiscStorage, createTable(_, _, _, _)).WillByDefault(Return(true));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, open()).Times(0);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, createTable(_, _, _, _)).Times(1);

    ASSERT_NE(nullptr, SpeakerManagerMiscStorage::create(m_stubMiscStorage));
}

TEST_F(SpeakerManagerMiscStorageTest, test_OpenedAndTableExists) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, open()).Times(0);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, createTable(_, _, _, _)).Times(0);

    ASSERT_NE(nullptr, SpeakerManagerMiscStorage::create(m_stubMiscStorage));
}

TEST_F(SpeakerManagerMiscStorageTest, test_GetPut) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    std::string jsonString;
    ON_CALL(*m_stubMiscStorage, put(_, _, _, _))
        .WillByDefault(
            Invoke([&jsonString](const std::string&, const std::string&, const std::string&, const std::string& data) {
                jsonString = data;
                return true;
            }));
    ON_CALL(*m_stubMiscStorage, get(_, _, _, _))
        .WillByDefault(
            Invoke([&jsonString](const std::string&, const std::string&, const std::string&, std::string* data) {
                *data = jsonString;
                return true;
            }));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);

    EXPECT_CALL(*m_stubMiscStorage, put(_, _, _, _)).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, get(_, _, _, _)).Times(1);

    SpeakerManagerStorageState state1 = {{10, false}, {20, true}};
    SpeakerManagerStorageState state2 = {{0, true}, {0, false}};

    auto storage = SpeakerManagerMiscStorage::create(m_stubMiscStorage);
    ASSERT_TRUE(storage->saveState(state1));
    ASSERT_TRUE(storage->loadState(state2));

    EXPECT_EQ(state1.speakerChannelState.channelVolume, state2.speakerChannelState.channelVolume);
    EXPECT_EQ(state1.speakerChannelState.channelMuteStatus, state2.speakerChannelState.channelMuteStatus);
    EXPECT_EQ(state1.alertsChannelState.channelVolume, state2.alertsChannelState.channelVolume);
    EXPECT_EQ(state1.alertsChannelState.channelMuteStatus, state2.alertsChannelState.channelMuteStatus);
}

TEST_F(SpeakerManagerMiscStorageTest, test_failedGet) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    ON_CALL(*m_stubMiscStorage, get(_, _, _, _))
        .WillByDefault(Invoke(
            [](const std::string&, const std::string&, const std::string&, std::string* data) { return false; }));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);

    EXPECT_CALL(*m_stubMiscStorage, get(_, _, _, _)).Times(1);

    SpeakerManagerStorageState state1 = {{10, false}, {20, true}};

    auto storage = SpeakerManagerMiscStorage::create(m_stubMiscStorage);
    ASSERT_FALSE(storage->loadState(state1));
}

TEST_F(SpeakerManagerMiscStorageTest, test_FailedPut) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    ON_CALL(*m_stubMiscStorage, put(_, _, _, _))
        .WillByDefault(Invoke(
            [](const std::string&, const std::string&, const std::string&, const std::string& data) { return false; }));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);

    EXPECT_CALL(*m_stubMiscStorage, put(_, _, _, _)).Times(1);

    SpeakerManagerStorageState state1 = {{10, false}, {20, true}};

    auto storage = SpeakerManagerMiscStorage::create(m_stubMiscStorage);
    ASSERT_FALSE(storage->saveState(state1));
}

TEST_F(SpeakerManagerMiscStorageTest, test_parseJson) {
    ON_CALL(*m_stubMiscStorage, isOpened()).WillByDefault(Return(true));
    ON_CALL(*m_stubMiscStorage, tableExists(_, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, bool* res) {
            *res = true;
            return true;
        }));

    ON_CALL(*m_stubMiscStorage, get(_, _, _, _))
        .WillByDefault(Invoke([](const std::string&, const std::string&, const std::string&, std::string* data) {
            *data = JSON_PAYLOAD;
            return true;
        }));

    EXPECT_CALL(*m_stubMiscStorage, isOpened()).Times(1);
    EXPECT_CALL(*m_stubMiscStorage, tableExists(_, _, _)).Times(1);

    EXPECT_CALL(*m_stubMiscStorage, get(_, _, _, _)).Times(1);

    SpeakerManagerStorageState state1 = {{10, true}, {20, false}};

    auto storage = SpeakerManagerMiscStorage::create(m_stubMiscStorage);
    ASSERT_TRUE(storage->loadState(state1));

    EXPECT_EQ(10, state1.speakerChannelState.channelVolume);
    EXPECT_FALSE(state1.speakerChannelState.channelMuteStatus);
    EXPECT_EQ(15, state1.alertsChannelState.channelVolume);
    EXPECT_TRUE(state1.alertsChannelState.channelMuteStatus);
}

}  // namespace test
}  // namespace speakerManager
}  // namespace alexaClientSDK
