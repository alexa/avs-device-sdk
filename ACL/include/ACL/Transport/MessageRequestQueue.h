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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUE_H_

#include <deque>
#include <memory>
#include <unordered_map>
#include <utility>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/functional/hash.h>

#include "ACL/Transport/MessageRequestQueueInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Class to manage @c MessageRequest send queues in HTTP2Transport.
 *
 * Note: This class is not thread safe. The user should ensure thread safety.
 */
class MessageRequestQueue : public MessageRequestQueueInterface {
public:
    /**
     * Constructor.
     */
    MessageRequestQueue();

    /// Override MessageRequestQueueInterface methods
    /// @{
    void enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest) override;
    avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>> peekRequestTime() override;
    std::shared_ptr<avsCommon::avs::MessageRequest> dequeueOldestRequest() override;
    std::shared_ptr<avsCommon::avs::MessageRequest> dequeueSendableRequest() override;
    bool isMessageRequestAvailable() const override;
    void setWaitingForSendAcknowledgement() override;
    void clearWaitingForSendAcknowledgement() override;
    bool empty() const override;
    void clear() override;
    /// @}

private:
    /// Flag indicating whether to block sending serialized messages.
    bool m_isWaitingForAcknowledgement;

    /// The queue of @c MessageRequests to be sent, paired with the time that each request was added to the queue.
    std::deque<
        std::pair<std::chrono::time_point<std::chrono::steady_clock>, std::shared_ptr<avsCommon::avs::MessageRequest>>>
        m_queue;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUE_H_
