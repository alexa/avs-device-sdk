/*
 * Copyright 2019-2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * An interface that abstracts the operations of a @c MessageRequestQueuesMap between the standard version
 * and the synchronized implementation.
 */
class MessageRequestQueueInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageRequestQueueInterface() = default;

    /**
     * Enqueues the @c MessageRequest to the corresponding send queue based on the send queue type.
     * If send queue type is not present, a new queue is created before enqueuing the request.
     */
    virtual void enqueueRequest(std::shared_ptr<avsCommon::avs::MessageRequest> messageRequest) = 0;

    /**
     * Peek at the next item in the queue and retrieve the time that the request was queued.
     *
     * @return The time that the next request (if any) was queued.
     */
    virtual avsCommon::utils::Optional<std::chrono::time_point<std::chrono::steady_clock>> peekRequestTime() = 0;

    /**
     * Dequeues the next available @c MessageRequest by taking into account if the queue is waiting for
     * a response. This method keeps track of the queue being processed and once a request from the queue
     * is dequeued moves it to the next queue. It also loops back to the starting queue at the end and exits
     * the loop if a valid MessageRequest is found or if it reaches the place it started.
     *
     * @return @c MessageRequest if an available, else return nullptr.
     */
    virtual std::shared_ptr<avsCommon::avs::MessageRequest> dequeueRequest() = 0;

    /**
     * This method checks if there is a @c MessageRequest available to be sent.
     *
     * @return true if @c MessageRequest is available to be sent, else false.
     */
    virtual bool isMessageRequestAvailable() const = 0;

    /**
     * Sets the waiting flag for the queue specified by the given send queue type.
     *
     * @param sendQueueType the send queue type of the queue to set the waiting flag on.
     */
    virtual void setWaitingFlagForQueue() = 0;

    /**
     * Clear the waiting flag for the queue specified by the given send queue type.
     *
     * @param sendQueueType the send queue type of the queue to clear the waiting flag on.
     */
    virtual void clearWaitingFlagForQueue() = 0;

    /**
     * Checks if there are any @c MessageRequests.
     *
     * @return true if there are no messageRequests in the queue, else false.
     */
    virtual bool empty() const = 0;

    /**
     * Clears all the @c MessageRequests along with the corresponding queues.
     */
    virtual void clear() = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEREQUESTQUEUEINTERFACE_H_
