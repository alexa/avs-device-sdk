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

#include "ACL/Transport/PostConnectSequencer.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating form this file.
static const std::string TAG("PostConnectSequencer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PostConnectSequencer> PostConnectSequencer::create(
    const PostConnectOperationsSet& postConnectOperations) {
    for (auto& postConnectOperation : postConnectOperations) {
        if (!postConnectOperation) {
            ACSDK_ERROR(LX("createFailed").d("reason", "invalid PostConnectOperation found"));
            return nullptr;
        }
    }
    return std::shared_ptr<PostConnectSequencer>(new PostConnectSequencer(postConnectOperations));
}

PostConnectSequencer::PostConnectSequencer(const PostConnectOperationsSet& postConnectOperations) :
        m_isStopping{false},
        m_postConnectOperations{postConnectOperations} {
}

PostConnectSequencer::~PostConnectSequencer() {
    ACSDK_DEBUG5(LX(__func__));
    stop();
}

bool PostConnectSequencer::doPostConnect(
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface> postConnectSender,
    std::shared_ptr<PostConnectObserverInterface> postConnectObserver) {
    ACSDK_DEBUG5(LX(__func__));

    if (!postConnectSender) {
        ACSDK_ERROR(LX("doPostConnectFailed").d("reason", "nullPostConnectSender"));
        return false;
    }

    if (!postConnectObserver) {
        ACSDK_ERROR(LX("doPostConnectFailed").d("reason", "nullPostConnectObserver"));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_mainLoopThread.joinable()) {
            ACSDK_ERROR(LX("doPostConnectFailed").d("reason", "mainLoop already running"));
            return false;
        }
        m_mainLoopThread = std::thread(&PostConnectSequencer::mainLoop, this, postConnectSender, postConnectObserver);
    }

    return true;
}

void PostConnectSequencer::mainLoop(
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface> postConnectSender,
    std::shared_ptr<PostConnectObserverInterface> postConnectObserver) {
    ACSDK_DEBUG5(LX(__func__));

    if (!postConnectSender) {
        ACSDK_ERROR(LX("mainLoopError").d("reason", "nullPostConnectSender"));
        return;
    }

    if (!postConnectObserver) {
        ACSDK_ERROR(LX("mainLoopError").d("reason", "nullPostConnectObserver"));
        return;
    }

    for (auto postConnectOperation : m_postConnectOperations) {
        {
            /// Set the current post connect operation.
            std::lock_guard<std::mutex> lock{m_mutex};
            if (m_isStopping) {
                ACSDK_DEBUG5(LX(__func__).m("stop called, exiting mainloop"));
                return;
            }
            m_currentPostConnectOperation = postConnectOperation;
        }

        if (!m_currentPostConnectOperation->performOperation(postConnectSender)) {
            /// Trigger post connect failure only when the perform operation failed.
            if (!isStopping() && postConnectObserver) {
                postConnectObserver->onUnRecoverablePostConnectFailure();
            }
            resetCurrentOperation();
            ACSDK_ERROR(LX(__func__).m("performOperation failed, exiting mainloop"));
            return;
        }
    }

    resetCurrentOperation();

    /// All post connect operations completed execution, notify the observer.
    if (postConnectObserver) {
        postConnectObserver->onPostConnected();
    }

    ACSDK_DEBUG5(LX("mainLoopReturning"));
}

void PostConnectSequencer::onDisconnect() {
    ACSDK_DEBUG5(LX(__func__));
    stop();
}

void PostConnectSequencer::resetCurrentOperation() {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_currentPostConnectOperation.reset();
}

bool PostConnectSequencer::isStopping() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isStopping;
}

void PostConnectSequencer::stop() {
    ACSDK_DEBUG5(LX(__func__));
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_isStopping) {
            return;
        }

        m_isStopping = true;
        if (m_currentPostConnectOperation) {
            m_currentPostConnectOperation->abortOperation();
        }
    }

    {
        std::lock_guard<std::mutex> lock{m_mainLoopThreadMutex};
        if (m_mainLoopThread.joinable()) {
            m_mainLoopThread.join();
        }
    }
}

}  // namespace acl
}  // namespace alexaClientSDK
