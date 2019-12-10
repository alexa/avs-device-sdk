/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_POSTCONNECTMESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_POSTCONNECTMESSAGEREQUEST_H_

#include <condition_variable>
#include <mutex>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * A specialized class to send post connect message requests.
 */
class PostConnectMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * Constructor.
     *
     * @param jsonContent The JSON content of the event.
     */
    PostConnectMessageRequest(const std::string& jsonContent);

    /// @name MessageRequest Functions
    /// @{
    void sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendMessageStatus) override;
    /// @}

    /**
     * A blocking call that waits for the message request's response to arrive.
     *
     * @return The @c MessageRequestObserverInterface::Status of the response.
     */
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status waitForCompletion();

    /**
     * Stops the message request processing and returns immediately.
     */
    void shutdown();

private:
    /// The status response of the message request.
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status m_sendMessageStatus;

    /// Flag indicating if the response has been received.
    bool m_responseReceived;

    /// Mutex to synchronize access to class variables.
    std::mutex m_requestMutex;

    /// Condition variable used to wake threads that are waiting.
    std::condition_variable m_requestCv;

    /// Flag indicating that the request has been requested to shutdown.
    bool m_isRequestShuttingDown;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_POSTCONNECTMESSAGEREQUEST_H_
