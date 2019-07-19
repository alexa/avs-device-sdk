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

#include <AndroidUtilities/AndroidSLESBufferQueue.h>
#include <AndroidUtilities/AndroidSLESObject.h>

/// String to identify log entries originating from this file.
static const std::string TAG{"AndroidSLESBufferQueue"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

AndroidSLESBufferQueue::AndroidSLESBufferQueue(
    std::shared_ptr<AndroidSLESObject> object,
    SLAndroidSimpleBufferQueueItf bufferQueue,
    std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> writer) :
        m_slObject{object},
        m_queueInterface{bufferQueue},
        m_writer{std::move(writer)},
        m_index{0} {
}

void AndroidSLESBufferQueue::onBufferCompleted() {
    // Write data and enqueue buffer.
    std::lock_guard<std::mutex> lock{m_mutex};
    m_writer->write(m_buffers[m_index].data(), m_buffers[m_index].size());
    enqueueBufferLocked();
}

static void recorderCallback(SLAndroidSimpleBufferQueueItf slQueue, void* bufferQueue) {
    static_cast<AndroidSLESBufferQueue*>(bufferQueue)->onBufferCompleted();
};

std::unique_ptr<AndroidSLESBufferQueue> AndroidSLESBufferQueue::create(
    std::shared_ptr<AndroidSLESObject> queueObject,
    std::unique_ptr<avsCommon::avs::AudioInputStream::Writer> writer) {
    // get the buffer queue interface
    SLAndroidSimpleBufferQueueItf queueInterface;
    if (!queueObject->getInterface(SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &queueInterface)) {
        ACSDK_ERROR(LX("initializeAndroidMicFailed").d("reason", "Failed to get buffer queue."));
        return nullptr;
    }

    auto bufferQueue = std::unique_ptr<AndroidSLESBufferQueue>(
        new AndroidSLESBufferQueue(queueObject, queueInterface, std::move(writer)));

    // register callback on the buffer queue
    if ((*queueInterface)->RegisterCallback(queueInterface, recorderCallback, bufferQueue.get()) != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("initializeAndroidMicFailed").d("reason", "Failed to register callback."));
        return nullptr;
    }

    return bufferQueue;
}

AndroidSLESBufferQueue::~AndroidSLESBufferQueue() {
    if (m_queueInterface) {
        if (!clearBuffers() ||
            (*m_queueInterface)->RegisterCallback(m_queueInterface, nullptr, nullptr) != SL_RESULT_SUCCESS) {
            ACSDK_WARN(LX("cleanBufferQueueFailed"));
        }
    }
}

bool AndroidSLESBufferQueue::enqueueBuffers() {
    std::lock_guard<std::mutex> lock{m_mutex};
    SLAndroidSimpleBufferQueueState state;
    auto result = (*m_queueInterface)->GetState(m_queueInterface, &state);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("enqueueBuffersFailed").d("reason", "getStateFailed").d("result", result));
        return false;
    }

    for (size_t i = state.count; i < NUMBER_OF_BUFFERS; ++i) {
        if (!enqueueBufferLocked()) {
            if (i == 0) {
                // Could not enqueue a single buffer.
                ACSDK_ERROR(LX("enqueueBuffersFailed").d("reason", "noBufferEnqueued"));
                return false;
            }
            ACSDK_WARN(LX("enqueueBuffersIncomplete").d("reason", "failedToEnqueueAllBuffers").d("enqueued", i));
            break;
        }
    }

    return true;
}

bool AndroidSLESBufferQueue::clearBuffers() {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto result = (*m_queueInterface)->Clear(m_queueInterface);
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("clearBuffersFailed").d("result", result));
        return false;
    }
    return true;
}

bool AndroidSLESBufferQueue::enqueueBufferLocked() {
    auto result =
        (*m_queueInterface)
            ->Enqueue(
                m_queueInterface, m_buffers[m_index].data(), m_buffers[m_index].size() * sizeof(m_buffers[m_index][0]));
    if (result != SL_RESULT_SUCCESS) {
        ACSDK_ERROR(LX("enqueueBufferFailed").d("result", result));
        return false;
    }
    m_index = (m_index + 1) % NUMBER_OF_BUFFERS;
    return true;
}

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
