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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEROUTEROBSERVER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEROUTEROBSERVER_H_

#include "ACL/Transport/MessageRouterObserverInterface.h"

#include <gmock/gmock.h>

#include <memory>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

// Cannot actually mock this class, since it is used exclusively through friend relationship
class MockMessageRouterObserver : public MessageRouterObserverInterface {
public:
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_notifiedOfReceive = false;
        m_notifiedOfStatusChanged = false;
    }

    bool wasNotifiedOfStatusChange() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_notifiedOfStatusChanged;
    }

    bool wasNotifiedOfReceive() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_notifiedOfReceive;
    }

    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status getLatestConnectionStatus() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_status;
    }

    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason getLatestConnectionChangedReason() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_reason;
    }

    bool waitForStatusChange(
        std::chrono::milliseconds timeout,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto result = m_cv.wait_for(lock, timeout, [this, status, reason]() {
            return m_notifiedOfStatusChanged && m_status == status && m_reason == reason;
        });
        return result;
    }

    std::string getLatestMessage() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_message;
    }

    std::string getAttachmentContextId() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_attachmentContextId;
    }

private:
    void onConnectionStatusChanged(
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        const std::vector<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::EngineConnectionStatus>&
            engineConnectionStatuses) override {
        if (engineConnectionStatuses.empty()) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        m_notifiedOfStatusChanged = true;
        m_status = status;
        for (auto connectionStatus : engineConnectionStatuses) {
            if (connectionStatus.engineType == avsCommon::sdkInterfaces::ENGINE_TYPE_ALEXA_VOICE_SERVICES) {
                m_reason = connectionStatus.reason;
            }
        }
        m_cv.notify_all();
    }

    void receive(const std::string& contextId, const std::string& message) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_notifiedOfReceive = true;
        m_attachmentContextId = contextId;
        m_message = message;
    }

    /// Mutex for serializing notification of status changes.
    std::mutex m_mutex;

    /// Condition variable used to wait for and notify of state change notifications.
    std::condition_variable m_cv;

    /// Last observed status
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status m_status;

    /// Last observerd reason
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason m_reason;

    /// Last observed context ID
    std::string m_attachmentContextId;

    /// Last observerd message
    std::string m_message;

    /// Whether the observer has been notified of a stage change.
    bool m_notifiedOfStatusChanged;

    /// Whether a receove was observed.
    bool m_notifiedOfReceive;
};

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEROUTEROBSERVER_H_
