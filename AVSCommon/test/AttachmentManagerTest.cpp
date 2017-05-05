/*
 * AttachmentManagerTest.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include <thread>

#include "AVSCommon/AttachmentManager.h"

namespace alexaClientSDK {
namespace avsCommon {

/// Time out to wait for an attachment.
static const std::chrono::milliseconds TIME_OUT_IN_MILLISECONDS(50);
/// The content ID of an attachment.
static const std::string CONTENT_ID_TEST0 = "contentId_test0";
/// The content ID of an attachment.
static const std::string CONTENT_ID_TEST1 = "contentId_test1";

using namespace ::testing;

/**
 * Test fixture class that tests the @c AttachmentManager
 */
class AttachmentManagerTest: public ::testing::Test {
protected:
    void SetUp() override {
        m_attachmentManager = std::make_shared<AttachmentManager>();
    }
    /// The attachment manager used for testing.
    std::shared_ptr<AttachmentManager> m_attachmentManager;
    /// The condition variable to ensure certain orders of reader and writer.
    std::condition_variable m_cv;
    /// Mutex to protect shared data.
    std::mutex m_mutex;
};

/// Test if the attachment manager has been successfully created.
TEST_F(AttachmentManagerTest, attachmentManagerCreation) {
    ASSERT_TRUE(m_attachmentManager);
}

/// Test if the attachment reader has timed out.
TEST_F(AttachmentManagerTest, createAttachmentReaderTimeOut) {
    auto attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST0);
    auto status = attachmentReader.wait_for(TIME_OUT_IN_MILLISECONDS);
    ASSERT_EQ(status, std::future_status::timeout);
}

/// Test getting attachment through attachment reader synchronously.
TEST_F(AttachmentManagerTest, createAttachmentReaderSynchrously) {
    std::shared_ptr<std::iostream> attachment = std::make_shared<std::stringstream>();
    m_attachmentManager->createAttachment(CONTENT_ID_TEST0, attachment);

    auto attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST0);
    ASSERT_TRUE(attachmentReader.get());
}

/// Test getting attachment through attachment reader asynchronously.
TEST_F(AttachmentManagerTest, gettingAttachmentAsynchrouslyWithWriterArrivesFirst) {
    auto createAttachment = [this]() {
        std::shared_ptr<std::iostream> attachment = std::make_shared<std::stringstream>();
        m_attachmentManager->createAttachment(CONTENT_ID_TEST0, attachment);
    };

    std::thread writeThread(createAttachment);
    auto attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST0);
    ASSERT_TRUE(attachmentReader.get());

    if (writeThread.joinable()) {
        writeThread.join();
    }
}

/// Test getting attachment through attachment reader asynchronously, with reader trying to read first.
TEST_F(AttachmentManagerTest, gettingAttachmentAsynchrouslyWithReaderArrivesFirst) {
    bool readerIsReady = false;

    auto createAttachment = [this, &readerIsReady]() {
        std::shared_ptr<std::iostream> attachment = std::make_shared<std::stringstream>();
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&readerIsReady]() {
            return readerIsReady;
        });
        m_attachmentManager->createAttachment(CONTENT_ID_TEST0, attachment);
    };

    auto readAttachment = [this, &readerIsReady]() {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST0);
        readerIsReady = true;
        lock.unlock();
        m_cv.notify_one();
        ASSERT_TRUE(attachmentReader.get());
    };

    // Start the read thread before the write thread.
    std::thread readThread(readAttachment);
    std::thread writeThread(createAttachment);

    if (readThread.joinable()) {
        readThread.join();
    }

    if (writeThread.joinable()) {
        writeThread.join();
    }
}

/// Test release the expired attachment upon the creation of another attachment.
TEST_F(AttachmentManagerTest, releaseExpiredAttachmentOnAttachmentCreation) {
    m_attachmentManager = std::make_shared<AttachmentManager>(std::chrono::minutes(0));
    m_attachmentManager->createAttachment(CONTENT_ID_TEST0, std::make_shared<std::stringstream>());
    m_attachmentManager->createAttachment(CONTENT_ID_TEST1, std::make_shared<std::stringstream>());

    auto attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST0);
    auto status = attachmentReader.wait_for(TIME_OUT_IN_MILLISECONDS);
    ASSERT_EQ(status, std::future_status::timeout);

    attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST1);
    status = attachmentReader.wait_for(TIME_OUT_IN_MILLISECONDS);
    ASSERT_EQ(status, std::future_status::timeout);
}

/// Test releasing attachment through attachment manager without any error.
TEST_F(AttachmentManagerTest, releaseAttachment) {
    m_attachmentManager->createAttachment(CONTENT_ID_TEST0, std::make_shared<std::stringstream>());

    m_attachmentManager->releaseAttachment(CONTENT_ID_TEST0);
    // Try to release the attachment that is not created;
    m_attachmentManager->releaseAttachment(CONTENT_ID_TEST1);
}

/// Test reading attachments after attachment has been released.
TEST_F(AttachmentManagerTest, readAttachmentAfterReleasing) {
    m_attachmentManager->createAttachment(CONTENT_ID_TEST0, std::make_shared<std::stringstream>());
    m_attachmentManager->createAttachment(CONTENT_ID_TEST1, std::make_shared<std::stringstream>());

    m_attachmentManager->releaseAttachment(CONTENT_ID_TEST0);
    m_attachmentManager->releaseAttachment(CONTENT_ID_TEST1);

    auto attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST0);
    auto status = attachmentReader.wait_for(TIME_OUT_IN_MILLISECONDS);
    ASSERT_EQ(status, std::future_status::timeout);

    attachmentReader = m_attachmentManager->createAttachmentReader(CONTENT_ID_TEST1);
    status = attachmentReader.wait_for(TIME_OUT_IN_MILLISECONDS);
    ASSERT_EQ(status, std::future_status::timeout);
}

} // namespace avsCommon
} // namespace alexaClientSDK
