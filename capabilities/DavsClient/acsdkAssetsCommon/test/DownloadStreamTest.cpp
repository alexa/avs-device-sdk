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

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <gtest/gtest.h>

#include "TestUtil.h"
#include "acsdkAssetsCommon/DownloadStream.h"

using namespace std;
using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::avsCommon::utils;

class DownloadStreamTest : public ::testing::Test {
public:
    void SetUp() override {
        DOWNLOAD_TEST_DIR = createTmpDir("temp");
    }

    void TearDown() override {
        filesystem::removeAll(DOWNLOAD_TEST_DIR);
    }

    std::string DOWNLOAD_TEST_DIR;
};

TEST_F(DownloadStreamTest, createReadOnlyFile) {
    auto readOnlyFile = "/etc/hosts";
    auto downloadStream = DownloadStream::create(readOnlyFile, 10);
    ASSERT_EQ(nullptr, downloadStream);
}

TEST_F(DownloadStreamTest, create) {
    auto tempFile = DOWNLOAD_TEST_DIR + "/temp";
    auto downloadStream = DownloadStream::create(tempFile, 10);
    ASSERT_TRUE(nullptr != downloadStream);

    auto tempData = "12345";
    ASSERT_TRUE(downloadStream->write(tempData, 5));
    ASSERT_TRUE(downloadStream->write(tempData, 1));
    ASSERT_FALSE(downloadStream->write(tempData, 5));
}

TEST_F(DownloadStreamTest, writeNullptr) {
    auto tempFile = DOWNLOAD_TEST_DIR + "/temp";
    auto downloadStream = DownloadStream::create(tempFile, 10);
    ASSERT_TRUE(nullptr != downloadStream);
    ASSERT_FALSE(downloadStream->write(nullptr, 1));
}

TEST_F(DownloadStreamTest, writeZeroByte) {
    auto tempFile = DOWNLOAD_TEST_DIR + "/temp";
    auto downloadStream = DownloadStream::create(tempFile, 0);
    ASSERT_TRUE(nullptr != downloadStream);
    ASSERT_TRUE(downloadStream->downloadSucceeded());

    auto tempData = "12345";
    ASSERT_TRUE(downloadStream->write(tempData, 0));
    ASSERT_TRUE(downloadStream->downloadSucceeded());
}

TEST_F(DownloadStreamTest, downloadSucceeded) {
    auto tempFile = DOWNLOAD_TEST_DIR + "/temp";
    auto downloadStream = DownloadStream::create(tempFile, 10);
    ASSERT_TRUE(nullptr != downloadStream);

    auto tempData = "12345";
    ASSERT_TRUE(downloadStream->write(tempData, 5));
    ASSERT_FALSE(downloadStream->downloadSucceeded());
    ASSERT_TRUE(downloadStream->write(tempData, 5));
    ASSERT_TRUE(downloadStream->downloadSucceeded());
}