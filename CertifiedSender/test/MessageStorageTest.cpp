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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CertifiedSender/SQLiteMessageStorage.h>
#include <SQLiteStorage/SQLiteStatement.h>
#include <AVSCommon/Utils/File/FileUtils.h>

#include <fstream>
#include <queue>
#include <memory>

using namespace ::testing;

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

using namespace avsCommon::utils::file;

/// The filename we will use for the test database file.
static const std::string TEST_DATABASE_FILE_PATH = "messageStorageTestDatabase.db";
/// The path delimiter used by the OS to identify file locations.
static const std::string PATH_DELIMITER = "/";
/// The full filepath to the database file we will create and delete during tests.
static std::string g_dbTestFilePath;
/// A test message text.
static const std::string TEST_MESSAGE_ONE = "test_message_one";
/// A test message text.
static const std::string TEST_MESSAGE_TWO = "test_message_two";
/// A test message text.
static const std::string TEST_MESSAGE_THREE = "test_message_three";
/// A test message uri.
static const std::string TEST_MESSAGE_URI = "/v20160207/events/SpeechRecognizer/Recognize";
/// The name of the alerts table.
static const std::string MESSAGES_TABLE_NAME = "messages_with_uri";
/// The name of the 'id' field we will use as the primary key in our tables.
static const std::string DATABASE_COLUMN_ID_NAME = "id";
/// The name of the 'message_text' field we will use as the primary key in our tables.
static const std::string DATABASE_COLUMN_MESSAGE_TEXT_NAME = "message_text";
/// The name of the 'uriPathExtension' field corresponding to the uri path extension of the message.
static const std::string DATABASE_COLUMN_URI = "uri";
/// The name of the 'timestamp' field is the creation time of the message.
static const std::string DATABASE_COLUMN_TIMESTAMP = "timestamp";
/// The SQL string to create the alerts table.
static const std::string CREATE_LEGACY_MESSAGES_TABLE_SQL_STRING =
    std::string("CREATE TABLE ") + MESSAGES_TABLE_NAME + " (" + DATABASE_COLUMN_ID_NAME + " INT PRIMARY KEY NOT NULL," +
    DATABASE_COLUMN_URI + " TEXT NOT NULL," + DATABASE_COLUMN_MESSAGE_TEXT_NAME + " TEXT NOT NULL);";
/**
 * A class which helps drive this unit test suite.
 */
class MessageStorageTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    MessageStorageTest();

    /**
     * Destructor.
     */
    ~MessageStorageTest();

    /**
     * Utility function to create the database, using the global filename.
     */
    void createDatabase();

    /**
     * Utility function to create legacy database that does not have timestamp column
     */
    bool createLegacyDatabase();

    /**
     * Utility function to cleanup the test database file, if it exists.
     */
    void cleanupLocalDbFile();

    /**
     * Utility function to check if the current table is legacy or not
     * EXPECT: 1 = DB is legacy, 0 = DB is not legacy, -1 = errors
     */
    int isDatabaseLegacy();

    /**
     * Utility function to insert old messages in the storage
     */
    bool insertOldMessagesToDatabase(int id, const std::string& record_age);

    /**
     * Utility function to create old messages in the storage
     */
    bool createOldMessages();

protected:
    /// The message database object we will test.
    std::shared_ptr<MessageStorageInterface> m_storage;
    std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteDatabase> m_legacyDB;
};

/**
 * Utility function to determine if the storage component is opened.
 *
 * @param storage The storage component to check.
 * @return True if the storage component's underlying database is opened, false otherwise.
 */
static bool isOpen(const std::shared_ptr<MessageStorageInterface>& storage) {
    std::queue<MessageStorageInterface::StoredMessage> dummyMessages;
    return storage->load(&dummyMessages);
}

MessageStorageTest::MessageStorageTest() : m_storage{std::make_shared<SQLiteMessageStorage>(g_dbTestFilePath)} {
    cleanupLocalDbFile();
}

MessageStorageTest::~MessageStorageTest() {
    m_storage->close();
    if (m_legacyDB) {
        m_legacyDB->close();
    }
    cleanupLocalDbFile();
}

void MessageStorageTest::createDatabase() {
    m_storage->createDatabase();
}

bool MessageStorageTest::createLegacyDatabase() {
    m_legacyDB = std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteDatabase>(
        new alexaClientSDK::storage::sqliteStorage::SQLiteDatabase(g_dbTestFilePath));

    if (!m_legacyDB || !m_legacyDB->initialize()) {
        return false;
    }

    if (!m_legacyDB->performQuery(CREATE_LEGACY_MESSAGES_TABLE_SQL_STRING)) {
        m_legacyDB->close();
        return false;
    }

    return true;
}

void MessageStorageTest::cleanupLocalDbFile() {
    if (g_dbTestFilePath.empty()) {
        return;
    }

    if (fileExists(g_dbTestFilePath)) {
        removeFile(g_dbTestFilePath.c_str());
    }
}

int MessageStorageTest::isDatabaseLegacy() {
    auto sqlStatement = m_legacyDB->createStatement("PRAGMA table_info(" + MESSAGES_TABLE_NAME + ");");

    if ((!sqlStatement) || (!sqlStatement->step())) {
        return -1;
    }

    const std::string tableInfoColumnName = "name";

    std::string columnName;
    while (SQLITE_ROW == sqlStatement->getStepResult()) {
        int columnCount = sqlStatement->getColumnCount();

        for (int i = 0; i < columnCount; i++) {
            std::string tableColumnName = sqlStatement->getColumnName(i);

            if (tableInfoColumnName == tableColumnName) {
                columnName = sqlStatement->getColumnText(i);
                if (DATABASE_COLUMN_TIMESTAMP == columnName) {
                    return 0;
                }
            }
        }

        if (!sqlStatement->step()) {
            return -1;
        }
    }
    return 1;
}

bool MessageStorageTest::insertOldMessagesToDatabase(int id, const std::string& record_age) {
    if (!m_legacyDB) {
        return false;
    }

    std::string sqlString = std::string("INSERT INTO " + MESSAGES_TABLE_NAME + " (") + DATABASE_COLUMN_ID_NAME + ", " +
                            DATABASE_COLUMN_URI + ", " + DATABASE_COLUMN_MESSAGE_TEXT_NAME + ", " +
                            DATABASE_COLUMN_TIMESTAMP + ") VALUES (?, ?, ?, datetime('now', ?));";

    auto statement = m_legacyDB->createStatement(sqlString);
    int boundParam = 1;
    std::string uriPathExtension = "testURI";
    std::string message = "testURI";
    if (!statement->bindIntParameter(boundParam++, id) ||
        !statement->bindStringParameter(boundParam++, uriPathExtension) ||
        !statement->bindStringParameter(boundParam++, message) ||
        !statement->bindStringParameter(boundParam, record_age)) {
        return false;
    }
    if (!statement->step()) {
        return false;
    }

    return true;
}

bool MessageStorageTest::createOldMessages() {
    m_legacyDB = std::unique_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteDatabase>(
        new alexaClientSDK::storage::sqliteStorage::SQLiteDatabase(g_dbTestFilePath));

    m_legacyDB->open();
    // Insert 60 records 1 month ago
    for (int id = 1; id <= 60; ++id) {
        if (!insertOldMessagesToDatabase(id, "-1 months")) {
            return false;
        }
    }

    // Insert 60 record at this moment
    for (int id = 61; id <= 120; ++id) {
        if (!insertOldMessagesToDatabase(id, "-0 seconds")) {
            return false;
        }
    }

    return true;
}
/**
 * Test basic construction.  Database should not be open.
 */
TEST_F(MessageStorageTest, test_constructionAndDestruction) {
    EXPECT_FALSE(isOpen(m_storage));
}

/**
 * Test database creation.
 */
TEST_F(MessageStorageTest, test_databaseCreation) {
    EXPECT_FALSE(isOpen(m_storage));
    createDatabase();
    EXPECT_TRUE(isOpen(m_storage));
}

/**
 * Test opening and closing a database.
 */
TEST_F(MessageStorageTest, test_openAndCloseDatabase) {
    EXPECT_FALSE(isOpen(m_storage));
    createDatabase();
    EXPECT_TRUE(isOpen(m_storage));
    m_storage->close();
    EXPECT_FALSE(isOpen(m_storage));
    EXPECT_TRUE(m_storage->open());
    EXPECT_TRUE(isOpen(m_storage));
    m_storage->close();
    EXPECT_FALSE(isOpen(m_storage));
}

/**
 * Test storing records in the database.
 */
TEST_F(MessageStorageTest, test_databaseStoreAndLoad) {
    createDatabase();
    EXPECT_TRUE(isOpen(m_storage));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 0);

    // test storing a single message first
    int dbId = 0;
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_ONE, &dbId));
    EXPECT_EQ(dbId, 1);
    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 1);
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);
    dbMessages.pop();

    // now store two more, and verify
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_TWO, &dbId));
    EXPECT_EQ(dbId, 2);
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_THREE, &dbId));
    EXPECT_EQ(dbId, 3);

    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 3);
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);
    dbMessages.pop();
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_TWO);
    dbMessages.pop();
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_THREE);
}

/**
 * Test erasing a record from the database.
 */
TEST_F(MessageStorageTest, test_databaseErase) {
    createDatabase();
    EXPECT_TRUE(isOpen(m_storage));

    // add three messages, and verify
    int dbId = 0;
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_ONE, &dbId));
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_TWO, &dbId));
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_THREE, &dbId));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 3);
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);

    // erase the first one, then verify it's gone from db
    EXPECT_TRUE(m_storage->erase(dbMessages.front().id));

    while (!dbMessages.empty()) {
        dbMessages.pop();
    }

    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 2);
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_TWO);
    dbMessages.pop();
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_THREE);
}

/**
 * Test clearing the database.
 */
TEST_F(MessageStorageTest, test_databaseClear) {
    createDatabase();
    EXPECT_TRUE(isOpen(m_storage));

    int dbId = 0;
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_ONE, &dbId));
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_TWO, &dbId));
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_THREE, &dbId));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 3);
    while (!dbMessages.empty()) {
        dbMessages.pop();
    }

    EXPECT_TRUE(m_storage->clearDatabase());

    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 0);
}

/**
 * Test storing records with URI in the database.
 */
TEST_F(MessageStorageTest, test_DatabaseStoreAndLoadWithURI) {
    createDatabase();
    EXPECT_TRUE(isOpen(m_storage));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(static_cast<int>(dbMessages.size()), 0);

    int dbId = 0;
    EXPECT_TRUE(m_storage->store(TEST_MESSAGE_ONE, TEST_MESSAGE_URI, &dbId));
    EXPECT_EQ(dbId, 1);
    EXPECT_TRUE(m_storage->load(&dbMessages));
    EXPECT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);
    EXPECT_EQ(dbMessages.front().uriPathExtension, TEST_MESSAGE_URI);
}

/**
 * Test legacy database
 */
TEST_F(MessageStorageTest, test_LegacyDatabase) {
    EXPECT_TRUE(createLegacyDatabase());
    EXPECT_EQ(isDatabaseLegacy(), 1);

    // Perform opening it to change it to new database
    // and check if it is legacy and it works
    EXPECT_TRUE(m_storage->open());
    EXPECT_TRUE(isOpen(m_storage));
    EXPECT_EQ(isDatabaseLegacy(), 0);

    // Close it and check the file
    m_storage->close();
    EXPECT_FALSE(isOpen(m_storage));
}

/**
 * Test erase legacy message
 */
TEST_F(MessageStorageTest, test_EraseMessageOverAgeAndSizeLimit) {
    createDatabase();
    // Create old messages over age and database size limit
    EXPECT_TRUE(createOldMessages());

    std::queue<MessageStorageInterface::StoredMessage> dbMessagesBefore;
    EXPECT_TRUE(m_storage->load(&dbMessagesBefore));
    EXPECT_EQ(static_cast<int>(dbMessagesBefore.size()), 120);

    // The mocking database opens and deletes over age and size limit
    m_storage->close();
    EXPECT_TRUE(m_storage->open());
    EXPECT_TRUE(isOpen(m_storage));

    std::queue<MessageStorageInterface::StoredMessage> dbMessagesAfter;
    EXPECT_TRUE(m_storage->load(&dbMessagesAfter));
    // eraseMessageOverAgeLimit() takes out the first 60 inserted 1 month ago
    // eraseMessageOverSizeLimit() takes out the next 35 messages

    EXPECT_EQ(static_cast<int>(dbMessagesAfter.size()), 25);
    for (int id = 96; id <= 120; ++id) {
        EXPECT_EQ(static_cast<int>(dbMessagesAfter.front().id), id);
        dbMessagesAfter.pop();
    }
    EXPECT_EQ(static_cast<int>(dbMessagesAfter.size()), 0);
}

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        std::cerr << "USAGE: " << std::string(argv[0]) << " <path_to_test_directory_location>" << std::endl;
        return 1;
    } else {
        alexaClientSDK::certifiedSender::test::g_dbTestFilePath =
            std::string(argv[1]) + alexaClientSDK::certifiedSender::test::PATH_DELIMITER +
            alexaClientSDK::certifiedSender::test::TEST_DATABASE_FILE_PATH;

        return RUN_ALL_TESTS();
    }
}
