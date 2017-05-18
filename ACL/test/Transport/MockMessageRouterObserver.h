/**
 * MockMessageRouterObserver.g
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
#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCK_MESSAGE_ROUTER_OBSERVER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCK_MESSAGE_ROUTER_OBSERVER_H_

#include <AVSCommon/AVS/Message.h>

#include "ACL/Transport/MessageRouterObserverInterface.h"

#include <gmock/gmock.h>

#include <memory>

namespace alexaClientSDK {
namespace acl {
namespace transport {

// Cannot actually mock this class, since it is used exclusively through friend relationship
class MockMessageRouterObserver: public MessageRouterObserverInterface {
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

    ConnectionStatus getLatestConnectionStatus() {
        return m_status;
    }

    ConnectionChangedReason getLatestConnectionChangedReason() {
        return m_reason;
    }

    std::shared_ptr<avsCommon::avs::Message> getLatestMessage() {
        return m_message;
    }

private:
    virtual void onConnectionStatusChanged(const ConnectionStatus status,
                                           const ConnectionChangedReason reason) override {
        notifiedOfStatusChanged = true;
        m_status = status;
        m_reason = reason;
    }
    virtual void receive(std::shared_ptr<avsCommon::avs::Message> msg) override {
        notifiedOfReceive = true;
        m_message = msg;
    }

    ConnectionStatus m_status;
    ConnectionChangedReason m_reason;
    std::shared_ptr<avsCommon::avs::Message> m_message;
    bool notifiedOfStatusChanged;
    bool notifiedOfReceive;
};

} // namespace transport
} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCK_MESSAGE_ROUTER_OBSERVER_H_
