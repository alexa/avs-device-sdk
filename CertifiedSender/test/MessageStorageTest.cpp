/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
/// The filename we will use for the test database file.
static const std::string TEST_SECOND_DATABASE_FILE_PATH = "messageStorageSecondTestDatabase.db";
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

/**
 * A class which helps drive this unit test suite.
 */
class MessageStorageTest : public ::testing::Test {
public:
    /**
     * Constructor.
     */
    MessageStorageTest() : m_storage{std::make_shared<SQLiteMessageStorage>(g_dbTestFilePath)} {
        cleanupLocalDbFile();
    }

    /**
     * Destructor.
     */
    ~MessageStorageTest() {
        m_storage->close();
        cleanupLocalDbFile();
    }

    /**
     * Utility function to create the database, using the global filename.
     */
    void createDatabase() {
        m_storage->createDatabase();
    }

    /**
     * Utility function to cleanup the test database file, if it exists.
     */
    void cleanupLocalDbFile() {
        if (g_dbTestFilePath.empty()) {
            return;
        }

        if (fileExists(g_dbTestFilePath)) {
            removeFile(g_dbTestFilePath.c_str());
        }
    }

protected:
    /// The message database object we will test.
    std::shared_ptr<MessageStorageInterface> m_storage;
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

/**
 * Test basic construction.  Database should not be open.
 */
TEST_F(MessageStorageTest, testConstructionAndDestruction) {
    ASSERT_FALSE(isOpen(m_storage));
}

/**
 * Test database creation.
 */
TEST_F(MessageStorageTest, testDatabaseCreation) {
    ASSERT_FALSE(isOpen(m_storage));
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));
}

/**
 * Test opening and closing a database.
 */
TEST_F(MessageStorageTest, testOpenAndCloseDatabase) {
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
 * Test storing records in the database.
 */
TEST_F(MessageStorageTest, testDatabaseStoreAndLoad) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 0);

    // test storing a single message first
    int dbId = 0;
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_ONE, &dbId));
    ASSERT_EQ(dbId, 1);
    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 1);
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);
    dbMessages.pop();

    // now store two more, and verify
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_TWO, &dbId));
    ASSERT_EQ(dbId, 2);
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_THREE, &dbId));
    ASSERT_EQ(dbId, 3);

    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 3);
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);
    dbMessages.pop();
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_TWO);
    dbMessages.pop();
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_THREE);
}

/**
 * Test erasing a record from the database.
 */
TEST_F(MessageStorageTest, testDatabaseErase) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    // add three messages, and verify
    int dbId = 0;
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_ONE, &dbId));
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_TWO, &dbId));
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_THREE, &dbId));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 3);
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_ONE);

    // erase the first one, then verify it's gone from db
    ASSERT_TRUE(m_storage->erase(dbMessages.front().id));

    while (!dbMessages.empty()) {
        dbMessages.pop();
    }

    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 2);
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_TWO);
    dbMessages.pop();
    ASSERT_EQ(dbMessages.front().message, TEST_MESSAGE_THREE);
}

/**
 * Test clearing the database.
 */
TEST_F(MessageStorageTest, testDatabaseClear) {
    createDatabase();
    ASSERT_TRUE(isOpen(m_storage));

    int dbId = 0;
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_ONE, &dbId));
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_TWO, &dbId));
    ASSERT_TRUE(m_storage->store(TEST_MESSAGE_THREE, &dbId));

    std::queue<MessageStorageInterface::StoredMessage> dbMessages;
    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 3);
    while (!dbMessages.empty()) {
        dbMessages.pop();
    }

    ASSERT_TRUE(m_storage->clearDatabase());

    ASSERT_TRUE(m_storage->load(&dbMessages));
    ASSERT_EQ(static_cast<int>(dbMessages.size()), 0);
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
