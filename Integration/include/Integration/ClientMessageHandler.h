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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CLIENTMESSAGEHANDLER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CLIENTMESSAGEHANDLER_H_

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>

namespace alexaClientSDK {
namespace integration {

using namespace alexaClientSDK::avsCommon::sdkInterfaces;

/// Minimal implementation of a message observer for integration tests.
class ClientMessageHandler : public MessageObserverInterface {
public:
    /// Constructor.
    ClientMessageHandler(std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

    /**
     * Implementation of the interface's receive function.
     *
     * For the purposes of these integration tests, this function simply logs and counts messages.
     */
    void receive(const std::string& contextId, const std::string& message) override;

    /**
     * Wait for a message to be received.
     *
     * This function waits for a specified number of seconds for a message to arrive.
     * @param duration Number of seconds to wait before giving up.
     * @return true if a message was received within the specified duration, else false.
     */
    bool waitForNext(const std::chrono::seconds duration = std::chrono::seconds(2));

private:
    /// Mutex to protect m_count.
    std::mutex m_mutex;
    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_wakeTrigger;
    /// Count of received messages that have not been waited on.
    unsigned int m_count;

    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
};

}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_CLIENTMESSAGEHANDLER_H_
