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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLEMESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLEMESSAGEREQUEST_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <AVSCommon/AVS/MessageRequest.h>

namespace alexaClientSDK {
namespace integration {

class ObservableMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * Constructor.
     */
    ObservableMessageRequest(
        const std::string& jsonContent,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr);

    void sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;

    void exceptionReceived(const std::string& exceptionMessage) override;

    /**
     * Utility function to get the status once the message has been sent.
     */
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status getSendMessageStatus() const;

    /**
     * Function to allow waiting for a particular status back from the component sending the message to AVS.
     */
    bool waitFor(
        const avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status,
        const std::chrono::seconds = std::chrono::seconds(10));
    /// Function indicating if sendCompleted has been called
    bool hasSendCompleted();
    /// Function indicating if exceptionReceived has been called
    bool wasExceptionReceived();

private:
    /// The status of whether the message was sent to AVS ok.
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status m_sendMessageStatus;
    /// Mutex used internally to enforce thread safety.
    mutable std::mutex m_mutex;
    /// The cv used when waiting for a particular status of a message being sent.
    std::condition_variable m_wakeTrigger;
    /// The variable that gets set when send is completed.
    std::atomic<bool> m_sendCompleted;
    /// The variable that gets set when exception is received.
    std::atomic<bool> m_exceptionReceived;
};

}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLEMESSAGEREQUEST_H_
