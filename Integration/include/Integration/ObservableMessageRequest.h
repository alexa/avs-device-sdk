/*
 * ObservableMessageRequest.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLE_MESSAGE_REQUEST_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLE_MESSAGE_REQUEST_H_

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
    ObservableMessageRequest(const std::string & jsonContent,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr);

    void onSendCompleted(avsCommon::avs::MessageRequest::Status) override;

    void onExceptionReceived(const std::string & exceptionMessage) override;

    /**
     * Utility function to get the status once the message has been sent.
     */
    avsCommon::avs::MessageRequest::Status getSendMessageStatus() const;

    /**
     * Function to allow waiting for a particular status back from the component sending the message to AVS.
     */
    bool waitFor(const avsCommon::avs::MessageRequest::Status, const std::chrono::seconds = std::chrono::seconds(10));

private:
    /// The status of whether the message was sent to AVS ok.
    avsCommon::avs::MessageRequest::Status m_sendMessageStatus;
    /// Mutex used internally to enforce thread safety.
    mutable std::mutex m_mutex;
    /// The cv used when waiting for a particular status of a message being sent.
    std::condition_variable m_wakeTrigger;
};

} // namespace integration
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_OBSERVABLE_MESSAGE_REQUEST_H_
