/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "ACL/Transport/HTTP2StreamPool.h"

namespace alexaClientSDK {
namespace acl {

/// String to identify log entries originating from this file.
static const std::string TAG("HTTP2StreamPool");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::utils;

unsigned int HTTP2StreamPool::m_nextStreamId = 1;

HTTP2StreamPool::HTTP2StreamPool(
    const int maxStreams,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager) :
        m_numAcquiredStreams{0},
        m_maxStreams{maxStreams},
        m_attachmentManager{attachmentManager} {
}

std::shared_ptr<HTTP2Stream> HTTP2StreamPool::createGetStream(
    const std::string& url,
    const std::string& authToken,
    std::shared_ptr<MessageConsumerInterface> messageConsumer) {
    std::shared_ptr<HTTP2Stream> stream = getStream(messageConsumer);
    if (!stream) {
        ACSDK_ERROR(LX("createGetStreamFailed").d("reason", "getStreamFailed"));
        return nullptr;
    }
    if (!stream->initGet(url, authToken)) {
        ACSDK_ERROR(LX("createGetStreamFailed").d("reason", "initGetFailed"));
        releaseStream(stream);
        return nullptr;
    }
    return stream;
}

std::shared_ptr<HTTP2Stream> HTTP2StreamPool::createPostStream(
    const std::string& url,
    const std::string& authToken,
    std::shared_ptr<avsCommon::avs::MessageRequest> request,
    std::shared_ptr<MessageConsumerInterface> messageConsumer) {
    std::shared_ptr<HTTP2Stream> stream = getStream(messageConsumer);
    if (!request) {
        ACSDK_ERROR(LX("createPostStreamFailed").d("reason", "nullMessageRequest"));
        return nullptr;
    }
    if (!stream) {
        ACSDK_ERROR(LX("createPostStreamFailed").d("reason", "getStreamFailed"));
        request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR);
        return nullptr;
    }
    if (!stream->initPost(url, authToken, request)) {
        ACSDK_ERROR(LX("createPostStreamFailed").d("reason", "initPostFailed"));
        request->sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status::INTERNAL_ERROR);
        releaseStream(stream);
        return nullptr;
    }
    return stream;
}

std::shared_ptr<HTTP2Stream> HTTP2StreamPool::getStream(std::shared_ptr<MessageConsumerInterface> messageConsumer) {
    if (!messageConsumer) {
        ACSDK_ERROR(LX("getStreamFailed").d("reason", "nullptrMessageConsumer"));
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_numAcquiredStreams >= m_maxStreams) {
        ACSDK_WARN(LX("getStreamFailed").d("reason", "maxStreamsAlreadyAcquired"));
        return nullptr;
    }

    std::shared_ptr<HTTP2Stream> result;
    if (m_pool.empty()) {
        result = std::make_shared<HTTP2Stream>(messageConsumer, m_attachmentManager);
    } else {
        result = m_pool.back();
        m_pool.pop_back();
    }
    m_numAcquiredStreams++;

    result->setLogicalStreamId(m_nextStreamId);
    // Increment by two so that these IDs tend to line up with the number at the end of x-amzn-requestId values.
    m_nextStreamId += 2;

    ACSDK_DEBUG0(
        LX("getStream").d("streamId", result->getLogicalStreamId()).d("numAcquiredStreams", m_numAcquiredStreams));
    return result;
}

void HTTP2StreamPool::releaseStream(std::shared_ptr<HTTP2Stream> stream) {
    if (!stream) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check to avoid releasing the same stream more than once accidentally
    for (auto item : m_pool) {
        if (item == stream) {
            ACSDK_ERROR(
                LX("releaseStreamFailed").d("reason", "alreadyReleased").d("streamId", stream->getLogicalStreamId()));
            return;
        }
    }

    m_numAcquiredStreams--;

    ACSDK_DEBUG0(
        LX("releaseStream").d("streamId", stream->getLogicalStreamId()).d("numAcquiredStreams", m_numAcquiredStreams));

    if (stream->reset()) {
        m_pool.push_back(stream);
    }
}

}  // namespace acl
}  // namespace alexaClientSDK
