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

#include <cstdlib>

#include <sys/time.h>

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
    struct timeval t;
    gettimeofday(&t, nullptr);
    std::string filePath = g_workingDirectory + "/SQLiteDatabaseTest-" + std::to_string(t.tv_sec) +
                           std::to_string(t.tv_usec) + std::to_string(rand());
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
