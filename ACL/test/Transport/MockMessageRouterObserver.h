/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
        notifiedOfReceive = false;
        notifiedOfStatusChanged = false;
    }

    bool wasNotifiedOfStatusChange() {
        return notifiedOfStatusChanged;
    }

    bool wasNotifiedOfReceive() {
        return notifiedOfReceive;
    }

    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status getLatestConnectionStatus() {
        return m_status;
    }

    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason getLatestConnectionChangedReason() {
        return m_reason;
    }

    std::string getLatestMessage() {
        return m_message;
    }

    std::string getAttachmentContextId() {
        return m_attachmentContextId;
    }

private:
    virtual void onConnectionStatusChanged(
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override {
        notifiedOfStatusChanged = true;
        m_status = status;
        m_reason = reason;
    }
    virtual void receive(const std::string& contextId, const std::string& message) override {
        notifiedOfReceive = true;
        m_attachmentContextId = contextId;
        m_message = message;
    }

    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status m_status;
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason m_reason;
    std::string m_attachmentContextId;
    std::string m_message;
    bool notifiedOfStatusChanged;
    bool notifiedOfReceive;
};

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEROUTEROBSERVER_H_
