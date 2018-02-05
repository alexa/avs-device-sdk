/*
 * MessageRequest.h
 *
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_

#include <memory>
#include <string>
#include <mutex>
#include <unordered_set>

#include "AVSCommon/AVS/Attachment/AttachmentReader.h"
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * This is a wrapper class which allows a client to send a Message to AVS, and be notified when the attempt to
 * send the Message was completed.
 */
class MessageRequest {
public:
    /**
     * Constructor.
     * @param jsonContent The message to be sent to AVS.
     * @param attachmentReader The attachment data (if present) to be sent to AVS along with the message.
     * Defaults to @c nullptr.
     */
    MessageRequest(
        const std::string& jsonContent,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr);

    /**
     * Destructor.
     */
    virtual ~MessageRequest();

    /**
     * Retrieves the JSON content to be sent to AVS.
     *
     * @return The JSON content to be sent to AVS.
     */
    std::string getJsonContent();

    /**
     * Retrieves the AttachmentReader of the Attachment data to be sent to AVS.
     *
     * @return The AttachmentReader of the Attachment data to be sent to AVS.
     */
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> getAttachmentReader();

    /**
     * This is called once the send request has completed.  The status parameter indicates success or failure.
     * @param status Whether the send request succeeded or failed.
     */
    virtual void sendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status);

    /**
     * This function will be called if AVS responds with an exception message to this message request being sent.
     *
     * @param exceptionMessage The exception message.
     */
    virtual void exceptionReceived(const std::string& exceptionMessage);

    /**
     * Add observer of MessageRequestObserverInterface.
     *
     * @param observer The observer to be added to the set.
     */
    void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface> observer);

    /**
     * Remove observer of MessageRequestObserverInterface.
     *
     * @param observer The observer to be removed from the set.
     */
    void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface> observer);

    /**
     * A function to evaluate if the given status reflects receipt of the message by the server.
     *
     * @param status The status being queried.
     * @return Whether the status reflects receipt of the message by the server.
     */
    static bool isServerStatus(sdkInterfaces::MessageRequestObserverInterface::Status status);

protected:
    /// Mutex to guard access of m_observers.
    std::mutex m_observerMutex;

    /// Set of observers of MessageRequestObserverInterface.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface>> m_observers;

    /// The JSON content to be sent to AVS.
    std::string m_jsonContent;

    /// The AttachmentReader of the Attachment data to be sent to AVS.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_attachmentReader;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_
