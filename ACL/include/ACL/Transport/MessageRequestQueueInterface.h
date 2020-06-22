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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUEINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUEINTERFACE_H_

#include <chrono>
#include <deque>
#include <memory>
#include <unordered_map>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace acl {

/**
 * An interface that abstracts queueing @c MessageRequests.
 */
class MessageRequestQueueInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageRequestQueueInterface() = default;

    /**
     * Enqueues a @c MessageRequest.
     */
    virtual void enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest) = 0;

    /**
     * Peek at the next item in the queue and retrieve the time that the request was queued.
     *
     * @return The time that the next request (if any) was queued.
     */
    virtual avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>> peekRequestTime() = 0;

    /**
     * Dequeues the oldest @c MessageRequest regardless of whether or not the queue is waiting for
     * a response.
     *
     * @return @c MessageRequest if available, else return nullptr.
     */
    virtual std::shared_ptr<avsCommon::avs::MessageRequest> dequeueOldestRequest() = 0;

    /**
     * Dequeues the next available @c MessageRequest taking into account if the queue is waiting for
     * a response to a previously sent message and if any messages that do not care about sequencing
     * are in the queue.
     *
     * @return @c MessageRequest if available, else return nullptr.
     */
    virtual std::shared_ptr<avsCommon::avs::MessageRequest> dequeueSendableRequest() = 0;

    /**
     * This method checks if there is a @c MessageRequest available to be sent.
     *
     * @return true if @c MessageRequest is available to be sent, else false.
     */
    virtual bool isMessageRequestAvailable() const = 0;

    /**
     * Sets the flag indicating that the queue is waiting for a send to be acknowledged.
     */
    virtual void setWaitingForSendAcknowledgement() = 0;

    /**
     * Clear the flag indicating that the queue is waiting for a send to be acknowledged.
     */
    virtual void clearWaitingForSendAcknowledgement() = 0;

    /**
     * Checks if there are any queued @c MessageRequests.
     *
     * @return true if there are no messageRequests in the queue, else false.
     */
    virtual bool empty() const = 0;

    /**
     * Clears all queue of @c MessageRequests.
     */
    virtual void clear() = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUEINTERFACE_H_
