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

#if defined(FILE_SYSTEM_UTILS_ENABLED)

#include <climits>
#include <fstream>
#include <gtest/gtest.h>

#include "AVSCommon/Utils/FileSystem/FileSystemUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace filesystem {
namespace test {

using namespace std;
using namespace ::testing;

class FileSystemUtilsTest : public ::testing::Test {
public:
    void SetUp() override {
        char dirName[L_tmpnam + 1]{};
        ASSERT_NE(nullptr, tmpnam(dirName));
        WORKING_DIR = dirName + string("_FileSystemUtilsTest/");

        ASSERT_FALSE(exists(WORKING_DIR));
        createDirectory(WORKING_DIR);
        ASSERT_TRUE(exists(WORKING_DIR));

#if defined(__linux__) || defined(__APPLE__)
        // on some OS, the temp path is symbolically linked, which can cause issues for prefix tests
        // to accommodate this, get the realpath of the temp directory
        char resolved_path[PATH_MAX + 1];
        ASSERT_NE(nullptr, ::realpath(WORKING_DIR.c_str(), resolved_path));
        WORKING_DIR = string(resolved_path);
        ASSERT_TRUE(exists(WORKING_DIR));
#endif

        ASSERT_FALSE(WORKING_DIR.empty());
        if (*WORKING_DIR.rbegin() != '/') {
            WORKING_DIR += "/";
        }
    }

    void TearDown() override {
        removeAll(WORKING_DIR);
        ASSERT_FALSE(exists(WORKING_DIR));
    }

    static void createFile(const string& filePath, const string& content = "defaultContent") {
        ofstream of(filePath);
        ASSERT_TRUE(of.good());
        of << content;
        of.close();
        ASSERT_TRUE(of.good());
    }

    static void createDirectory(const string& dirPath) {
        makeDirectory(dirPath);
        ASSERT_TRUE(exists(dirPath));
    }

    static string unifyDelimiter(string path) {
        replace(path.begin(), path.end(), '\\', '/');
        return path;
    };

    string WORKING_DIR;
};

TEST_F(FileSystemUtilsTest, testChangingFilePermissions) {
    auto path = WORKING_DIR + "file.txt";
    auto originalContent = "testing";
    auto updatedContent = "updated_testing";
    string content;
    ifstream reader;
    ofstream writer;

    // setup test file with content
    writer.open(path);
    writer << originalContent;
    writer.close();
    ASSERT_TRUE(exists(path));

#ifndef _WIN32  // all files in windows are readable
    // giving the file write only permission makes it impossible for us to read it
    ASSERT_TRUE(changePermissions(path, OWNER_WRITE));
    reader.open(path);
    ASSERT_FALSE(reader.good());
#endif

    // changing the permissions to read only will allow us then to read
    ASSERT_TRUE(changePermissions(path, OWNER_READ));
    reader.open(path);
    ASSERT_TRUE(reader.good());
    reader >> content;
    ASSERT_EQ(content, originalContent);
    reader.close();

    // however, with read only permission, we cannot then write
    writer.open(path);
    ASSERT_FALSE(writer.good());

    // finally, giving the file read/write permission allows us to both update it and read it again
    ASSERT_TRUE(changePermissions(path, OWNER_WRITE | OWNER_READ));
    writer.open(path);
    ASSERT_TRUE(writer.good());
    writer << updatedContent;
    writer.close();

    reader.open(path);
    ASSERT_TRUE(reader.good());
    reader >> content;
    ASSERT_EQ(content, updatedContent);
    reader.close();
}

TEST_F(FileSystemUtilsTest, testExistsValidatesThatAFileOrDirectoryExists) {  // NOLINT
    auto file = WORKING_DIR + "file";
    auto directory = WORKING_DIR + "directory";

    ASSERT_FALSE(exists(file));
    ASSERT_FALSE(exists(directory));
    createFile(file);
    createDirectory(directory);
    ASSERT_TRUE(exists(file));
    ASSERT_TRUE(exists(directory));
}

TEST_F(FileSystemUtilsTest, testMovingFileToNewPath) {  // NOLINT
    auto directoryBefore = WORKING_DIR + "directory/";
    auto directoryAfter = WORKING_DIR + "newDirectory/";
    auto fileBefore = WORKING_DIR + "file";
    auto fileAfter = directoryBefore + "newFileName";

    createDirectory(directoryBefore);
    createFile(fileBefore);
    ASSERT_TRUE(exists(directoryBefore));
    ASSERT_TRUE(exists(fileBefore));

    ASSERT_TRUE(move(fileBefore, fileAfter));
    ASSERT_FALSE(exists(fileBefore));
    ASSERT_TRUE(exists(fileAfter));

    ASSERT_TRUE(move(directoryBefore, directoryAfter));
    ASSERT_FALSE(exists(directoryBefore));
    ASSERT_TRUE(exists(directoryAfter));
}

TEST_F(FileSystemUtilsTest, testCheckingDiskSpace) {  // NOLINT
    ASSERT_GT(availableSpace(WORKING_DIR), 0UL);
    ASSERT_EQ(availableSpace("/some/non/existing/directory"), 0UL);
}

TEST_F(FileSystemUtilsTest, testCheckingSizeOfFilesAndDirectory) {  // NOLINT
    auto subDirectory = WORKING_DIR + "directory/";
    auto file1 = WORKING_DIR + "file1";
    auto file2 = subDirectory + "file2";
    string fileContent = "This is some text to fill into the file that's being created";

    createDirectory(subDirectory);
    createFile(file1, fileContent);
    createFile(file2, fileContent);
    ASSERT_EQ(sizeOf(file1), fileContent.size());
    ASSERT_EQ(sizeOf(file2), fileContent.size());
    ASSERT_EQ(sizeOf(WORKING_DIR), fileContent.size() * 2);
}

TEST_F(FileSystemUtilsTest, testThatCurrentDirectoryExists) {  // NOLINT
    auto dir = currentDirectory();
    ASSERT_FALSE(dir.empty());
    ASSERT_TRUE(exists(dir));
}

TEST_F(FileSystemUtilsTest, testMakeDirectory) {  // NOLINT
    Permissions mode = OWNER_WRITE | OWNER_READ | OWNER_EXEC;
    auto simpleDirName = WORKING_DIR + "simple-dir-name";
    auto recursiveCreate = WORKING_DIR + "first-directory/second-directory/third-directory";
    auto repeatedSlash = WORKING_DIR + "before-double-slash//after-double-slash";
    auto recursiveCreateWithSlashAtEnd = WORKING_DIR + "slash/at/the/end/";
    auto filePath = WORKING_DIR + "file";
    auto filePathFollowedByDir = WORKING_DIR + "file/some/dir";
    createFile(filePath);

    ASSERT_TRUE(makeDirectory(simpleDirName));
    ASSERT_TRUE(exists(simpleDirName));
    ASSERT_TRUE(makeDirectory(simpleDirName));
    ASSERT_TRUE(makeDirectory(recursiveCreate, mode));
    ASSERT_TRUE(exists(recursiveCreate));
    ASSERT_TRUE(makeDirectory(repeatedSlash, mode));
    ASSERT_TRUE(exists(repeatedSlash));
    ASSERT_TRUE(makeDirectory(recursiveCreateWithSlashAtEnd, mode));
    ASSERT_TRUE(exists(recursiveCreateWithSlashAtEnd));
    ASSERT_FALSE(makeDirectory(filePath, mode));
    ASSERT_FALSE(makeDirectory(filePathFollowedByDir, mode));
    ASSERT_FALSE(makeDirectory(WORKING_DIR + "first-directory/../this-fails", mode));
    ASSERT_FALSE(makeDirectory(WORKING_DIR + "first-directory/./this-fails", mode));
    ASSERT_FALSE(makeDirectory(""));
}

TEST_F(FileSystemUtilsTest, testPathContainsPrefix) {  // NOLINT
    auto prefix = WORKING_DIR + "davs";
    createDirectory(prefix);
    string good_path = prefix + "/valid_locale";
    string ok_path = prefix + "/valid_locale/../still/valid";
    string minimal_ok_path = prefix;
    ASSERT_TRUE(pathContainsPrefix(good_path, prefix));
    ASSERT_TRUE(pathContainsPrefix(ok_path, prefix));
    ASSERT_TRUE(pathContainsPrefix(minimal_ok_path, prefix));

    string sneaky_bad_path = prefix + "/../../system/bin";
    string flagrant_bad_path = "/system/bin";
    string invalid_bad_path = "&*$)#%^*(";
    ASSERT_FALSE(pathContainsPrefix(sneaky_bad_path, prefix));
    ASSERT_FALSE(pathContainsPrefix(flagrant_bad_path, prefix));
    ASSERT_FALSE(pathContainsPrefix(invalid_bad_path, prefix));
}

TEST_F(FileSystemUtilsTest, testFileBasename) {  // NOLINT
    EXPECT_EQ(basenameOf("/tmp/file.txt"), "file.txt");
    EXPECT_EQ(basenameOf("/tmp/directory"), "directory");
    EXPECT_EQ(basenameOf("/tmp/directory/"), "directory");
    EXPECT_EQ(basenameOf("/tmp/directory//"), "directory");
    EXPECT_EQ(basenameOf("/tmp"), "tmp");
    EXPECT_EQ(basenameOf("tmp/"), "tmp");
    EXPECT_EQ(basenameOf("tmp"), "tmp");
    EXPECT_EQ(basenameOf("tmp///"), "tmp");
    EXPECT_EQ(basenameOf("/t"), "t");
    EXPECT_EQ(basenameOf("t/"), "t");
    EXPECT_EQ(basenameOf("/"), "");
    EXPECT_EQ(basenameOf("////"), "");
    EXPECT_EQ(basenameOf("/some/.."), "..");
    EXPECT_EQ(basenameOf("/some/."), ".");
    EXPECT_EQ(basenameOf(".."), "..");
    EXPECT_EQ(basenameOf("."), ".");
    EXPECT_EQ(basenameOf(""), "");

#ifdef _WIN32
    // Windows is able to accept '\\' as well as '/' delimiters
    EXPECT_EQ(basenameOf("\\tmp\\directory\\"), "directory");
    EXPECT_EQ(basenameOf("C:\\tmp\\directory"), "directory");
#endif
}

TEST_F(FileSystemUtilsTest, testPathDirname) {  // NOLINT
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/tmp/file.txt")), "/tmp/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/tmp/directory")), "/tmp/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/tmp/directory/")), "/tmp/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/tmp/directory//")), "/tmp/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/tmp")), "/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("tmp/")), "./");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("tmp")), "./");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("tmp///")), "./");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/t")), "/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("t/")), "./");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/")), "/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("////")), "/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/some/..")), "/some/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("/some/.")), "/some/");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("..")), "./");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf(".")), "./");
    EXPECT_EQ(unifyDelimiter(parentDirNameOf("")), "./");

#ifdef _WIN32
    // Windows is able to accept '\\' as well as '/' delimiters
    EXPECT_EQ(parentDirNameOf("C:\\tmp/path"), "C:\\tmp\\");
    EXPECT_EQ(parentDirNameOf("C:/tmp/path"), "C:\\tmp\\");
    EXPECT_EQ(parentDirNameOf("C:/"), "C:\\");
    EXPECT_EQ(parentDirNameOf("C:"), "C:\\");
#endif
}

TEST_F(FileSystemUtilsTest, testListOfDifferentKinds) {  // NOLINT
    string file1 = "file1";
    string file2 = "file2";
    string dir1 = "dir1";
    string dir2 = "dir2";
    string link = "link";
    createFile(WORKING_DIR + file1);
    createFile(WORKING_DIR + file2);
    createDirectory(WORKING_DIR + dir1);
    createDirectory(WORKING_DIR + dir2);

    auto files = list(WORKING_DIR, FileType::REGULAR_FILE);
    auto directories = list(WORKING_DIR, FileType::DIRECTORY);
    auto all = list(WORKING_DIR, FileType::ALL);
    auto def = list(WORKING_DIR);

    ASSERT_EQ(all, def);
    ASSERT_EQ(all.size(), 4UL);
    ASSERT_EQ(files.size(), 2UL);
    ASSERT_EQ(directories.size(), 2UL);

    ASSERT_TRUE(find(all.begin(), all.end(), file1) != all.end());
    ASSERT_TRUE(find(all.begin(), all.end(), file2) != all.end());
    ASSERT_TRUE(find(all.begin(), all.end(), dir1) != all.end());
    ASSERT_TRUE(find(all.begin(), all.end(), dir2) != all.end());

    ASSERT_TRUE(find(files.begin(), files.end(), file1) != files.end());
    ASSERT_TRUE(find(files.begin(), files.end(), file2) != files.end());

    ASSERT_TRUE(find(directories.begin(), directories.end(), dir1) != directories.end());
    ASSERT_TRUE(find(directories.begin(), directories.end(), dir2) != directories.end());
}

TEST_F(FileSystemUtilsTest, testRemoveAllFilesAndOrDirectories) {  // NOLINT
    string file = "file.txt";
    string directory = "dir";
    string fullDirectory = "fulldir";

    createFile(WORKING_DIR + file);
    createDirectory(WORKING_DIR + directory);
    createDirectory(WORKING_DIR + fullDirectory + "/" + fullDirectory + "/" + fullDirectory);
    createFile(WORKING_DIR + fullDirectory + "/" + fullDirectory + "/" + file);
    createFile(WORKING_DIR + fullDirectory + "/" + file);

    ASSERT_TRUE(exists(WORKING_DIR + file));
    ASSERT_TRUE(removeAll(WORKING_DIR + file));
    ASSERT_FALSE(exists(WORKING_DIR + file));

    ASSERT_TRUE(exists(WORKING_DIR + directory));
    ASSERT_TRUE(removeAll(WORKING_DIR + directory));
    ASSERT_FALSE(exists(WORKING_DIR + directory));

    ASSERT_TRUE(exists(WORKING_DIR + fullDirectory));
    ASSERT_TRUE(removeAll(WORKING_DIR + fullDirectory));
    ASSERT_FALSE(exists(WORKING_DIR + fullDirectory));

    ASSERT_TRUE(removeAll(WORKING_DIR + file));
    ASSERT_FALSE(exists(WORKING_DIR + file));
}

}  // namespace test
}  // namespace filesystem
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // FILE_SYSTEM_UTILS_ENABLED