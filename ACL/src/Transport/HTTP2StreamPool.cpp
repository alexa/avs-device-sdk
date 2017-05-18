/*
 * HTTP2StreamPool.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "AVSUtils/Logging/Logger.h"
#include "ACL/Transport/HTTP2StreamPool.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsUtils;

    HTTP2StreamPool::HTTP2StreamPool(
        const int maxStreams,
        std::shared_ptr<avsCommon::AttachmentManagerInterface> attachmentManager)
        : m_numRemovedStreams{0},
          m_maxStreams{maxStreams},
          m_attachmentManager{attachmentManager} {
}

std::shared_ptr<HTTP2Stream> HTTP2StreamPool::createGetStream(const std::string& url, const std::string& authToken,
        MessageConsumerInterface *messageConsumer) {
    std::shared_ptr<HTTP2Stream> stream = getStream(messageConsumer);
    if (!stream) {
        Logger::log("Could not get stream from stream pool");
        releaseStream(stream);
        return nullptr;
    }
    if (!stream->initGet(url, authToken)) {
        Logger::log("Could not setup stream for get request");
        releaseStream(stream);
        return nullptr;
    }
    return stream;
}

std::shared_ptr<HTTP2Stream> HTTP2StreamPool::createPostStream(const std::string& url, const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> request, MessageConsumerInterface *messageConsumer) {
    std::shared_ptr<HTTP2Stream> stream = getStream(messageConsumer);
    if (!stream) {
        Logger::log("Could not get stream from stream pool");
        request->onSendCompleted(avsCommon::avs::MessageRequest::Status::INTERNAL_ERROR);
        return nullptr;
    }
    if (!stream->initPost(url, authToken, request)) {
        Logger::log("Could not setup stream for post request");
        request->onSendCompleted(avsCommon::avs::MessageRequest::Status::INTERNAL_ERROR);
        releaseStream(stream);
        return nullptr;
    }
    return stream;
}

std::shared_ptr<HTTP2Stream> HTTP2StreamPool::getStream(MessageConsumerInterface *messageConsumer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_numRemovedStreams >= m_maxStreams) {
        return nullptr;
    }
    m_numRemovedStreams++;
    if (m_pool.empty()) {
        return std::make_shared<HTTP2Stream>(messageConsumer, m_attachmentManager);
    }

    std::shared_ptr<HTTP2Stream> ret = m_pool.back();
    m_pool.pop_back();
    return ret;
}

void HTTP2StreamPool::releaseStream(std::shared_ptr<HTTP2Stream> stream) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (stream->reset()) {
        m_pool.push_back(stream);
    }
    m_numRemovedStreams--;
}

} // acl
} // alexaClientSDK
