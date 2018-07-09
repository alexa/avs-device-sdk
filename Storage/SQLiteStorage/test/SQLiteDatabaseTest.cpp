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

#include <chrono>
#include <cstdlib>

#include <gtest/gtest.h>

#include <AVSCommon/Utils/File/FileUtils.h>
#include <SQLiteStorage/SQLiteDatabase.h>

namespace alexaClientSDK {
namespace storage {
namespace sqliteStorage {
namespace test {

/// Variable for storing the working directory.  This is where all of the test databases will be created.
static std::string g_workingDirectory;

/// An example of a path that doesn't exist in a system.
static const std::string BAD_PATH =
    "_/_/_/there/is/no/way/this/path/should/exist/,/so/it/should/cause/an/error/when/creating/the/db";

/**
 * Helper function that generates a unique filepath using the passed in g_workingDirectory.
 *
 * @return A unique filepath.
 */
static std::string generateDbFilePath() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto nanosecond = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime.time_since_epoch()).count());
    std::string filePath =
        g_workingDirectory + "/SQLiteDatabaseTest-" + std::to_string(nanosecond) + std::to_string(rand());
    EXPECT_FALSE(avsCommon::utils::file::fileExists(filePath));
    return filePath;
}

/// Test to close DB then open it.
TEST(SQLiteDatabaseTest, CloseThenOpen) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db(dbFilePath);
    ASSERT_TRUE(db.initialize());
    db.close();
    ASSERT_TRUE(db.open());
}

/// Test to initialize already existing DB.
TEST(SQLiteDatabaseTest, InitializeAlreadyExisting) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db1(dbFilePath);
    ASSERT_TRUE(db1.initialize());

    SQLiteDatabase db2(dbFilePath);
    ASSERT_FALSE(db2.initialize());

    db2.close();
    db1.close();
}

/// Test to initialize a bad path.
TEST(SQLiteDatabaseTest, InitializeBadPath) {
    SQLiteDatabase db(BAD_PATH);
    ASSERT_FALSE(db.initialize());
}

/// Test to initialize a directory.
TEST(SQLiteDatabaseTest, InitializeOnDirectory) {
    SQLiteDatabase db(g_workingDirectory);
    ASSERT_FALSE(db.initialize());
}

/// Test to initialize DB twice.
TEST(SQLiteDatabaseTest, InitializeTwice) {
    SQLiteDatabase db(generateDbFilePath());

    ASSERT_TRUE(db.initialize());
    ASSERT_FALSE(db.initialize());
    db.close();
}

/// Test to open already existing DB.
TEST(SQLiteDatabaseTest, OpenAlreadyExisting) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db1(dbFilePath);
    ASSERT_TRUE(db1.initialize());

    SQLiteDatabase db2(dbFilePath);
    ASSERT_TRUE(db2.open());

    db2.close();
    db1.close();
}

/// Test to open a bad path.
TEST(SQLiteDatabaseTest, OpenBadPath) {
    SQLiteDatabase db(BAD_PATH);
    ASSERT_FALSE(db.open());
}

/// Test to open directory.
TEST(SQLiteDatabaseTest, OpenDirectory) {
    SQLiteDatabase db(g_workingDirectory);
    ASSERT_FALSE(db.open());
}

/// Test to open DB twice.
TEST(SQLiteDatabaseTest, OpenTwice) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db1(dbFilePath);
    ASSERT_TRUE(db1.initialize());

    SQLiteDatabase db2(dbFilePath);
    ASSERT_TRUE(db2.open());
    ASSERT_FALSE(db2.open());

    db2.close();
    db1.close();
}

/// Test transactions commit
TEST(SQLiteDatabaseTest, TransactionsCommit) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db(dbFilePath);
    ASSERT_TRUE(db.initialize());

    {
        auto transaction1 = db.beginTransaction();
        ASSERT_NE(transaction1, nullptr);
        ASSERT_TRUE(transaction1->commit());
    }

    // Should not fail because previous transaction is completed
    auto transaction2 = db.beginTransaction();
    ASSERT_NE(transaction2, nullptr);

    db.close();
}

// Test transactions rollback
TEST(SQLiteDatabaseTest, TransactionsRollback) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db(dbFilePath);
    ASSERT_TRUE(db.initialize());

    {
        auto transaction1 = db.beginTransaction();
        ASSERT_NE(transaction1, nullptr);
        ASSERT_TRUE(transaction1->rollback());
    }

    // Should not fail because previous transaction is completed
    auto transaction2 = db.beginTransaction();
    ASSERT_NE(transaction2, nullptr);

    db.close();
}

/// Test nested transactions
TEST(SQLiteDatabaseTest, NestedTransactions) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db(dbFilePath);
    ASSERT_TRUE(db.initialize());

    auto transaction1 = db.beginTransaction();
    ASSERT_NE(transaction1, nullptr);
    auto transaction2 = db.beginTransaction();
    ASSERT_EQ(transaction2, nullptr);

    db.close();
}

/// Test transactions double commit
TEST(SQLiteDatabaseTest, DoubleCommit) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db(dbFilePath);
    ASSERT_TRUE(db.initialize());

    auto transaction1 = db.beginTransaction();
    ASSERT_NE(transaction1, nullptr);
    ASSERT_TRUE(transaction1->commit());
    ASSERT_FALSE(transaction1->commit());

    db.close();
}

/// Test automatic rollback
TEST(SQLiteDatabaseTest, AutoRollback) {
    auto dbFilePath = generateDbFilePath();
    SQLiteDatabase db(dbFilePath);
    ASSERT_TRUE(db.initialize());

    {
        auto transaction1 = db.beginTransaction();
        ASSERT_NE(transaction1, nullptr);
    }

    // Should not fail because transaction should have been automatically completed
    auto transaction2 = db.beginTransaction();
    ASSERT_NE(transaction2, nullptr);

    db.close();
}

}  // namespace test
}  // namespace sqliteStorage
}  // namespace storage
}  // namespace alexaClientSDK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0]) << " <path to folder for test>" << std::endl;
        return -1;
    } else {
        alexaClientSDK::storage::sqliteStorage::test::g_workingDirectory = std::string(argv[1]);
        return RUN_ALL_TESTS();
    }
}
