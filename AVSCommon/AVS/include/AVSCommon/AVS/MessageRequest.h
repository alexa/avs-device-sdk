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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_

#include <cstdlib>
#include <functional>
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

class EditableMessageRequest;

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

    /// A struct to hold event namespace and name.
    struct EventHeaders {
        /**
         * Constructor.
         *
         * @param eventNamespace The namespace of the event.
         * @param eventName The name of the event.
         */
        EventHeaders(const std::string& eventNamespace, const std::string& eventName) :
                eventNamespace{eventNamespace},
                eventName{eventName} {
        }

        EventHeaders() = default;

        /// The event namespace.
        std::string eventNamespace;

        /// The event name.
        std::string eventName;
    };

    /**
     * Function to resolve an editable message request based on the provided resolveKey by updating the MessageRequest.
     * @param[in,out] req Target editable request message that will be modified in place.
     * @param resolveKey Key used to resolve the message request
     * @return @c true if resolving successfully, else @ false
     * @note This function need to be thread-safe, and is allowed to block.
     */
    using MessageRequestResolveFunction =
        std::function<bool(const std::shared_ptr<EditableMessageRequest>& req, const std::string& resolveKey)>;

    /**
     * Constructor.
     *
     * @param jsonContent The message to be sent to AVS.
     * @param uriPathExtension An optional uri path extension which will be appended to the base url of the AVS.
     * endpoint.  If not specified, the default AVS path extension should be used by the sender implementation.
     * @param threshold. An optional threshold to ACL to send the metric specified by streamMetricName. If this isn't
     * specified no metric will be recorded.
     * @param streamMetricName. An optional metric name for ACL to submit when the threshold is met. If this isn't
     * specified no metric will be recorded.
     */
    MessageRequest(
        const std::string& jsonContent,
        const std::string& uriPathExtension = "",
        const unsigned int threshold = 0,
        const std::string& streamMetricName = "");

    /**
     * Constructor.
     *
     * @param jsonContent The message to be sent to AVS.
     * @param threshold. A required threshold to ACL to send the metric specified by streamMetricName.
     * @param streamMetricName. A required metric name for ACL to submit when the threshold is met.
     */
    MessageRequest(const std::string& jsonContent, const unsigned int threshold, const std::string& streamMetricName);

    /**
     * Constructor.
     *
     * @param jsonContent The message to be sent to AVS.
     * @param isSerialized True if sending this message must be serialized with sending other serialized messages.
     * @param uriPathExtension An optional uri path extension which will be appended to the base url of the AVS.
     * @param headers key/value pairs of extra HTTP headers to use with this request.
     * endpoint.  If not specified, the default AVS path extension should be used by the sender implementation.
     * @param resolver Function to resolve message. Null if message doesn't need resolving. Resolving function aims to
     * support the use case that one message request will be sent to multiple places with some fields having different
     * values for different destinations. In such use cases, @c MessageRequest works as a container with all required
     * info to build different versions of requests. The resolving function contains the logic to build the target
     * message request based on the info in the original request, and provided resolveKey.
     * @param threshold. An optional threshold to ACL to send the metric specified by streamMetricName. If this isn't
     * specified no metric will be recorded.
     * @param streamMetricName. An optional metric name for ACL to submit when the threshold is met. If this isn't
     * specified no metric will be recorded.
     */
    MessageRequest(
        const std::string& jsonContent,
        bool isSerialized,
        const std::string& uriPathExtension = "",
        std::vector<std::pair<std::string, std::string>> headers = {},
        MessageRequestResolveFunction resolver = nullptr,
        const unsigned int threshold = 0,
        const std::string& streamMetricName = "");

    /**
     * Constructor to construct a MessageRequest which contains a copy of the data in @c MessageRequest.
     * @param messageRequest MessageRequest to copy from
     * @note Observers are not considered data and don't get copied by this constructor.
     */
    MessageRequest(const MessageRequest& messageRequest);

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
    std::string getJsonContent() const;

    /**
     * Return true if sending this message must be serialized with sending other serialized messages.
     *
     * @return True if sending this message must be serialized with sending other serialized messages.
     */
    bool getIsSerialized() const;

    /**
     * Retrieves the path extension to be appended to the base URL when sending.
     *
     * @return The path extension to be appended to the base URL when sending.
     */
    std::string getUriPathExtension() const;

    /**
     * Gets the number of @c AttachmentReaders in this message.
     *
     * @return The number of readers in this message.
     */
    int attachmentReadersCount() const;

    /**
     * Retrieves the ith AttachmentReader in the message.
     *
     * @param index The index of the @c AttachmentReader to retrieve.
     * @return @c NamedReader of the ith attachment in the message.
     * A null pointer is returned when @c index is out of bound.
     */
    std::shared_ptr<NamedReader> getAttachmentReader(size_t index) const;

    /**
     * Called when the Response code is received.
     *
     * @param status The status of the response that was received.
     */
    virtual void responseStatusReceived(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status);

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
     * Retrieve MessageRequest event headers (namespace and name).
     *
     * @return EventHeaders containing the namespace and name.
     */
    EventHeaders retrieveEventHeaders() const;

    /**
     * Get additional HTTP headers for this request
     *
     * @return key/value pairs of extra HTTP headers to use with this request.
     */
    const std::vector<std::pair<std::string, std::string>>& getHeaders() const;

    /**
     * Check whether message is resolved and ready to send.
     * @return @c true if message is already resolved, else @c false
     */
    bool isResolved() const;

    /**
     * Resolve message to a valid message by updating the content of the message based on provided resolveKey
     * @param resolveKey Key used to resolve message
     * @return New resolved MessageRequest
     */
    std::shared_ptr<MessageRequest> resolveRequest(const std::string& resolveKey) const;

    /**
     * Get the stream bytes threshold, to determine when we should record the stream metric.
     * @return m_threshold
     */
    unsigned int getStreamBytesThreshold() const;

    /**
     * Get the name for the bytes stream metric.
     * @return m_streamMetricName
     */
    std::string getStreamMetricName() const;

protected:
    /// Mutex to guard access of m_observers.
    std::mutex m_observerMutex;

    /// Set of observers of MessageRequestObserverInterface.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface>> m_observers;

    /// The JSON content to be sent to AVS.
    std::string m_jsonContent;

    /// True if sending this message must be serialized with sending other serialized messages.
    bool m_isSerialized;

    /// The path extension to be appended to the base URL when sending.
    std::string m_uriPathExtension;

    /// The AttachmentReaders of the Attachments data to be sent to AVS.
    std::vector<std::shared_ptr<NamedReader>> m_readers;

    /// Optional headers to send with this request to AVS.
    std::vector<std::pair<std::string, std::string>> m_headers;

    /// Resolver function to resolve current message request to a valid state. Null if message is already resolved.
    MessageRequestResolveFunction m_resolver;

    /// The name for the stream byte metric.
    std::string m_streamMetricName;

    /// The threshold for the number of bytes for when we should record the stream metric.
    unsigned int m_streamBytesThreshold;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_MESSAGEREQUEST_H_
