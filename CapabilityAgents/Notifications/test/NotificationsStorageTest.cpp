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

#include <memory>
#include <random>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Notifications/SQLiteNotificationsStorage.h>
#include <Notifications/NotificationIndicator.h>

#include <AVSCommon/Utils/File/FileUtils.h>

using namespace ::testing;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace notifications {
namespace test {

using namespace avsCommon::utils::file;
using IndicatorState = avsCommon::avs::IndicatorState;

/// The filename we will use for the test database file.
static const std::string TEST_DATABASE_FILE_PATH = "notificationsStorageTestDatabase.db";

/// The path delimiter used by the OS to identify file locations.
static const std::string PATH_DELIMITER = "/";

/// AssetId strings for testing
static const std::string TEST_ASSET_ID1 = "testAssetId1";
static const std::string TEST_ASSET_ID2 = "testAssetId2";

/// AssetUrl strings for testing
static const std::string TEST_ASSET_URL1 = "testAssetUrl1";
static const std::string TEST_ASSET_URL2 = "testAssetUrl2";

/// Indicator state table/column name in the database.
static const std::string INDICATOR_STATE_NAME = "indicatorState";

/// Value to represent the invalid indicator state.
static const int INVALID_STATE_VALUE = 123;

/// Number to use when generating many NotificationIndicators.
static const unsigned int NUM_TEST_INDICATORS = 15;

/// Seed to generate random values for NotificationIndicators.
static const unsigned int NOTIFICATION_INDICATOR_SEED = 1;

/// The max random number to generate.
static const unsigned int MAX_RANDOM_INT = 100;

/**
 * Utility function to determine if the storage component is opened.
 *
 * @param storage The storage component to check.
 * @return True if the storage component's underlying database is opened, false otherwise.
 */
static bool isOpen(const std::shared_ptr<NotificationsStorageInterface>& storage) {
    int dummySize;
    return storage->getQueueSize(&dummySize);
}

class NotificationsStorageTest : public ::testing::Test {
public:
    NotificationsStorageTest();

    ~NotificationsStorageTest();

    /**
     * Utility function to create the database, using the global filename.
     */
    void createDatabase();

    /**
     * Utility function to cleanup the test database file, if it exists.
     */
    void cleanupLocalDbFile();

    void checkNotificationIndicatorsEquality(
        const NotificationIndicator& actual,
        const NotificationIndicator& expected);

protected:
    /// The message database object we will test.
    std::shared_ptr<SQLiteNotificationsStorage> m_storage;
};

NotificationsStorageTest::NotificationsStorageTest() :
        m_storage{std::make_shared<SQLiteNotificationsStorage>(TEST_DATABASE_FILE_PATH)} {
    cleanupLocalDbFile();
}

NotificationsStorageTest::~NotificationsStorageTest() {
    m_storage->close();
    cleanupLocalDbFile();
}

void NotificationsStorageTest::createDatabase() {
    m_storage->createDatabase();
}

void NotificationsStorageTest::cleanupLocalDbFile() {
    if (fileExists(TEST_DATABASE_FILE_PATH)) {
        removeFile(TEST_DATABASE_FILE_PATH.c_str());
    }
}

void NotificationsStorageTest::checkNotificationIndicatorsEquality(
    const NotificationIndicator& actual,
    const NotificationIndicator& expected) {
    ASSERT_EQ(actual.persistVisualIndicator, expected.persistVisualIndicator);
    ASSERT_EQ(actual.playAudioIndicator, expected.playAudioIndicator);
    ASSERT_EQ(actual.asset.assetId, expected.asset.assetId);
    ASSERT_EQ(actual.asset.url, expected.asset.url);
}

/**
 * Test basic construction. Database should not be open.
 */
TEST_F(NotificationsStorageTest, test_constructionAndDestruction) {
    ASSERT_FALSE(isOpen(m_storage));
}

/**
 * Test database creation.
 */
TEST_F(NotificationsStorageTest, test_databaseCreation) {
    ASSERT_FALSE(isOpen(m_storage));
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));
}

/**
 * Test opening and closing a database.
 */
TEST_F(NotificationsStorageTest, test_openAndCloseDatabase) {
    ASSERT_FALSE(isOpen(m_storage));
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));
    m_storage->close();
    ASSERT_FALSE(isOpen(m_storage));
    ASSERT_TRUE(m_storage->open());
    ASSERT_TRUE(isOpen(m_storage));
    m_storage->close();
    ASSERT_FALSE(isOpen(m_storage));
}

/**
 * Test enqueueing and dequeueing records in the database.
 */
TEST_F(NotificationsStorageTest, test_databaseEnqueueAndDequeue) {
    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);
    NotificationIndicator secondIndicator(false, true, TEST_ASSET_ID2, TEST_ASSET_URL2);

    // should fail to enqueue/dequeue if database is not open for business
    ASSERT_FALSE(m_storage->enqueue(firstIndicator));
    ASSERT_FALSE(m_storage->dequeue());

    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->enqueue(secondIndicator));

    NotificationIndicator firstDequeue;
    ASSERT_TRUE(m_storage->peek(&firstDequeue));
    ASSERT_TRUE(m_storage->dequeue());
    // should match the first indicator
    checkNotificationIndicatorsEquality(firstDequeue, firstIndicator);

    // one more for good measure
    NotificationIndicator secondDequeue;
    ASSERT_TRUE(m_storage->peek(&secondDequeue));
    ASSERT_TRUE(m_storage->dequeue());
    // should match the second indicator
    checkNotificationIndicatorsEquality(secondDequeue, secondIndicator);

    // dequeue should fail if there is nothing left to dequeue
    ASSERT_FALSE(m_storage->dequeue());
}

/**
 * Test setting and getting the IndicatorState
 */
TEST_F(NotificationsStorageTest, test_settingAndGettingIndicatorState) {
    IndicatorState state;

    // should fail to set/get if database is not open for business
    ASSERT_FALSE(m_storage->setIndicatorState(IndicatorState::ON));
    ASSERT_FALSE(m_storage->getIndicatorState(&state));

    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    ASSERT_TRUE(m_storage->setIndicatorState(IndicatorState::ON));
    ASSERT_TRUE(m_storage->getIndicatorState(&state));

    ASSERT_EQ(state, IndicatorState::ON);

    ASSERT_TRUE(m_storage->setIndicatorState(IndicatorState::OFF));
    ASSERT_TRUE(m_storage->getIndicatorState(&state));

    ASSERT_EQ(state, IndicatorState::OFF);
}

/**
 * Test just clearing the notification indicators table.
 */
TEST_F(NotificationsStorageTest, test_clearingNotificationIndicators) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);
    NotificationIndicator secondIndicator(false, true, TEST_ASSET_ID2, TEST_ASSET_URL2);

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->enqueue(secondIndicator));
    ASSERT_TRUE(m_storage->clearNotificationIndicators());

    ASSERT_FALSE(m_storage->dequeue());
}

/**
 * Test that empty database (due to a corruption or crash) results in default indicator state being used
 * (non-undefined).
 */
TEST_F(NotificationsStorageTest, test_defaultValueForEmptyStorage) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->clearNotificationIndicators());

    IndicatorState indicatorState = IndicatorState::UNDEFINED;
    ASSERT_TRUE(m_storage->getIndicatorState(&indicatorState));
    ASSERT_TRUE(IndicatorState::UNDEFINED != indicatorState);
}

/**
 * Test that invalid database value (due to a corruption or crash) results in default indicator state being used
 * (non-undefined).
 */
TEST_F(NotificationsStorageTest, test_defaultValueForInvalidDBContents) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));

    // Setup direct access to the DB.
    alexaClientSDK::storage::sqliteStorage::SQLiteDatabase database(TEST_DATABASE_FILE_PATH);
    ASSERT_TRUE(database.open());

    std::string sqlString = "UPDATE " + INDICATOR_STATE_NAME + " SET " + INDICATOR_STATE_NAME + " = (?);";

    auto updateStatement = database.createStatement(sqlString);

    ASSERT_THAT(updateStatement, NotNull());

    ASSERT_TRUE(updateStatement->bindIntParameter(1, INVALID_STATE_VALUE));

    ASSERT_TRUE(updateStatement->step());

    updateStatement->finalize();

    IndicatorState indicatorState = IndicatorState::UNDEFINED;
    ASSERT_TRUE(m_storage->getIndicatorState(&indicatorState));
    ASSERT_TRUE(IndicatorState::UNDEFINED != indicatorState);
}

/**
 * Test checking for an empty queue.
 */
TEST_F(NotificationsStorageTest, test_checkingEmptyQueue) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    bool empty = false;

    // should start out empty
    ASSERT_TRUE(m_storage->checkForEmptyQueue(&empty));
    ASSERT_TRUE(empty);

    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);
    NotificationIndicator secondIndicator(false, true, TEST_ASSET_ID2, TEST_ASSET_URL2);

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->enqueue(secondIndicator));

    // should not be empty anymore
    ASSERT_TRUE(m_storage->checkForEmptyQueue(&empty));
    ASSERT_FALSE(empty);

    ASSERT_TRUE(m_storage->dequeue());

    // only dequeued once, should still contain a record
    ASSERT_TRUE(m_storage->checkForEmptyQueue(&empty));
    ASSERT_FALSE(empty);

    ASSERT_TRUE(m_storage->dequeue());

    // should finally be empty again
    ASSERT_TRUE(m_storage->checkForEmptyQueue(&empty));
    ASSERT_TRUE(empty);
}

/**
 * Test persistence across closing and reopening database.
 */
TEST_F(NotificationsStorageTest, test_databasePersistence) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);
    NotificationIndicator secondIndicator(false, true, TEST_ASSET_ID2, TEST_ASSET_URL2);

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->enqueue(secondIndicator));

    m_storage->close();

    ASSERT_FALSE(isOpen(m_storage));
    ASSERT_TRUE(m_storage->open());
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator firstDequeue;
    ASSERT_TRUE(m_storage->peek(&firstDequeue));
    ASSERT_TRUE(m_storage->dequeue());
    // should match the first indicator
    checkNotificationIndicatorsEquality(firstDequeue, firstIndicator);

    // let's try closing again before the second dequeue
    m_storage->close();

    ASSERT_FALSE(isOpen(m_storage));
    ASSERT_TRUE(m_storage->open());
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator secondDequeue;
    ASSERT_TRUE(m_storage->peek(&secondDequeue));
    ASSERT_TRUE(m_storage->dequeue());
    // should match the second indicator
    checkNotificationIndicatorsEquality(secondDequeue, secondIndicator);
}

/**
 * Test that ordering is maintained with multiple queueing/dequeueing.
 */
TEST_F(NotificationsStorageTest, test_queueOrder) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    // generate ints with a random generator, this will be used to produce random bool values
    std::default_random_engine intGenerator;
    intGenerator.seed(NOTIFICATION_INDICATOR_SEED);

    std::uniform_int_distribution<int>(1, MAX_RANDOM_INT);

    std::vector<NotificationIndicator> notificationIndicators;

    // generate a bunch of random NotificationIndicators and enqueue them
    for (unsigned int i = 0; i < NUM_TEST_INDICATORS; i++) {
        // populate the new NotificationIndicator with random values
        bool r_persistVisualIndicator = intGenerator() % 2;
        bool r_playAudioIndicator = intGenerator() % 2;
        std::string r_assetId = intGenerator() % 2 ? TEST_ASSET_ID1 : TEST_ASSET_ID2;
        std::string r_assetUrl = intGenerator() % 2 ? TEST_ASSET_URL1 : TEST_ASSET_URL2;

        NotificationIndicator ni(r_persistVisualIndicator, r_playAudioIndicator, r_assetId, r_assetUrl);
        notificationIndicators.push_back(ni);
        m_storage->enqueue(ni);
    }

    NotificationIndicator dequeuePtr;

    // dequeue all the NotificationIndicators and check that they match the random ones previously generated
    for (unsigned int i = 0; i < NUM_TEST_INDICATORS; i++) {
        ASSERT_TRUE(m_storage->peek(&dequeuePtr));
        ASSERT_TRUE(m_storage->dequeue());
        checkNotificationIndicatorsEquality(dequeuePtr, notificationIndicators[i]);
    }
}

/**
 * Test that peek() functionality works.
 */
TEST_F(NotificationsStorageTest, test_peek) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);
    NotificationIndicator secondIndicator(false, true, TEST_ASSET_ID2, TEST_ASSET_URL2);

    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->enqueue(secondIndicator));

    NotificationIndicator peekedAt;
    ASSERT_TRUE(m_storage->peek(&peekedAt));

    // should match the first indicator
    checkNotificationIndicatorsEquality(peekedAt, firstIndicator);
    ASSERT_TRUE(m_storage->dequeue());

    // let's peek again, this time expecting the second indicator
    ASSERT_TRUE(m_storage->peek(&peekedAt));
    // should match the first indicator
    checkNotificationIndicatorsEquality(peekedAt, secondIndicator);
}

/**
 * Test that queueSize() works correctly.
 */

TEST_F(NotificationsStorageTest, test_size) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    int size = 0;
    ASSERT_TRUE(m_storage->getQueueSize(&size));
    ASSERT_EQ(size, 0);

    // test size after adding a few indicators
    NotificationIndicator firstIndicator(true, false, TEST_ASSET_ID1, TEST_ASSET_URL1);
    ASSERT_TRUE(m_storage->enqueue(firstIndicator));
    ASSERT_TRUE(m_storage->getQueueSize(&size));
    ASSERT_EQ(size, 1);

    NotificationIndicator secondIndicator(false, true, TEST_ASSET_ID2, TEST_ASSET_URL2);
    ASSERT_TRUE(m_storage->enqueue(secondIndicator));
    ASSERT_TRUE(m_storage->getQueueSize(&size));
    ASSERT_EQ(size, 2);

    // and now pop everything off, checking size at every step
    ASSERT_TRUE(m_storage->dequeue());
    ASSERT_TRUE(m_storage->getQueueSize(&size));
    ASSERT_EQ(size, 1);

    ASSERT_TRUE(m_storage->dequeue());
    ASSERT_TRUE(m_storage->getQueueSize(&size));
    ASSERT_EQ(size, 0);
}

}  // namespace test
}  // namespace notifications
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 1) {
        std::cerr << "USAGE: " << std::string(argv[0]) << std::endl;
        return 1;
    }
    return RUN_ALL_TESTS();
}
