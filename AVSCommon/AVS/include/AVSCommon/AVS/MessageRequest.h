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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_

#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

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
    /// A struct to hold an @c AttachmentReader alongside its name.
    struct NamedReader {
        /**
         * Constructor.
         *
         * @param name The name of the message part.
         * @param reader The @c AttachmentReader holding the data for this part.
         */
        NamedReader(const std::string& name, std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader) :
                name{name},
                reader{reader} {
        }

        /// The name of this message part.
        std::string name;

        /// The @c AttachmentReader contains the data of this message part.
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> reader;
    };

    /**
     * Constructor.
     *
     * @param jsonContent The message to be sent to AVS.
     * @param uriPathExtension An optional uri path extension which will be appended to the base url of the AVS.
     * endpoint.  If not specified, the default AVS path extension should be used by the sender implementation.
     */
    MessageRequest(const std::string& jsonContent, const std::string& uriPathExtension = "");

    /**
     * Destructor.
     */
    virtual ~MessageRequest();

    /**
     * Adds an attachment reader to the message. The attachment data will be the next
     * part in the message to be sent to AVS.
     * @note: The order by which the message attachments sent to AVS
     * is the one by which they have been added to it.
     *
     * @param name The name of the message part containing the attachment data.
     * @param attachmentReader The attachment data to be sent to AVS along with the message.
     */
    void addAttachmentReader(const std::string& name, std::shared_ptr<attachment::AttachmentReader> attachmentReader);

    /**
     * Retrieves the JSON content to be sent to AVS.
     *
     * @return The JSON content to be sent to AVS.
     */
    std::string getJsonContent();

    /**
     * Retrieves the path extension to be appended to the base URL when sending.
     *
     * @return The path extension to be appended to the base URL when sending.
     */
    std::string getUriPathExtension();

    /**
     * Gets the number of @c AttachmentReaders in this message.
     *
     * @return The number of readers in this message.
     */
    int attachmentReadersCount();

    /**
     * Retrieves the ith AttachmentReader in the message.
     *
     * @param index The index of the @c AttachmentReader to retrieve.
     * @return @c NamedReader of the ith attachment in the message.
     * A null pointer is returned when @c index is out of bound.
     */
    std::shared_ptr<NamedReader> getAttachmentReader(size_t index);

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

    /// The path extension to be appended to the base URL when sending.
    std::string m_uriPathExtension;

    /// The AttachmentReaders of the Attachments data to be sent to AVS.
    std::vector<std::shared_ptr<NamedReader>> m_readers;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_
