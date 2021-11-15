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

#include <memory>

#include "acsdkAssetsCommon/DownloadChunkQueue.h"

using namespace std;
using namespace alexaClientSDK::acsdkAssets::common;

class DownloadChunkQueueTest : public ::testing::Test {
public:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(DownloadChunkQueueTest, queueSizeZero) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(0));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_EQ(1u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushEmptyBuffer) {
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_FALSE(queue->push(nullptr, 1));
    ASSERT_EQ(0u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushZeroByte) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_FALSE(queue->push(data, 0));
    ASSERT_EQ(0u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushSizeOver) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_TRUE(queue->push(data, 2));
    ASSERT_FALSE(queue->push(data, 100));
    ASSERT_EQ(2u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushCompleteMisMatch) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_FALSE(queue->pushComplete(true));
    ASSERT_EQ(1u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushCompleteMatch) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_TRUE(queue->push(data, 7));
    ASSERT_TRUE(queue->pushComplete(true));
    ASSERT_EQ(2u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushAbort) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_FALSE(queue->pushComplete(false));
    ASSERT_EQ(1u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushCompleteAbort) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_TRUE(queue->push(data, 7));
    ASSERT_FALSE(queue->pushComplete(false));
    ASSERT_EQ(2u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pop) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_EQ(1u, queue->size());
    ASSERT_TRUE(nullptr != queue->waitAndPop());
    ASSERT_EQ(0u, queue->size());
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_TRUE(nullptr != queue->waitAndPop());

    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_TRUE(queue->push(data, 5));
    ASSERT_EQ(2u, queue->size());
    ASSERT_TRUE(nullptr != queue->waitAndPop());
    ASSERT_TRUE(nullptr != queue->waitAndPop());
    ASSERT_EQ(0u, queue->size());

    ASSERT_TRUE(queue->pushComplete(true));
    ASSERT_TRUE(queue->popComplete(true));
}

TEST_F(DownloadChunkQueueTest, popAfterPushAbort) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_FALSE(queue->pushComplete(false));
    ASSERT_EQ(nullptr, queue->waitAndPop());
    ASSERT_FALSE(queue->popComplete(true));
    ASSERT_EQ(1u, queue->size());
}

TEST_F(DownloadChunkQueueTest, pushAfterPopAbort) {
    char data[16] = {0};
    shared_ptr<DownloadChunkQueue> queue(new DownloadChunkQueue(8));
    ASSERT_TRUE(queue != nullptr);
    ASSERT_TRUE(queue->push(data, 1));
    ASSERT_FALSE(queue->popComplete(false));
    ASSERT_FALSE(queue->push(data, 1));
    ASSERT_EQ(1u, queue->size());
}