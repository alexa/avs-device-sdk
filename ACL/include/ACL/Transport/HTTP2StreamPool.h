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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2STREAMPOOL_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2STREAMPOOL_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/MessageRequest.h>

#include "ACL/Transport/HTTP2Stream.h"
#include "ACL/Transport/MessageConsumerInterface.h"

namespace alexaClientSDK {
namespace acl {

class HTTP2StreamPool {
public:
    /**
     * Default constructor
     *
     * @params maxStreams The maximum number of streams that can be active
     * @params attachmentManager The attachment manager.
     */
    HTTP2StreamPool(
        const int maxStreams,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager);

    /**
     * Grabs an HTTP2Stream from the pool and configures it to be an HTTP GET.
     *
     * @param url The request URL.
     * @param authToken The LWA token to supply.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @return An HTTP2Stream from the stream pool, or @c nullptr if there was an error.
     */
    std::shared_ptr<HTTP2Stream> createGetStream(
        const std::string& url,
        const std::string& authToken,
        std::shared_ptr<MessageConsumerInterface> messageConsumer);

    /**
     * Grabs an HTTP2Stream from the pool and configures it to be an HTTP POST.
     *
     * @param url The request URL.
     * @param authToken The LWA token to supply.
     * @param request The message request.
     * @param messageConsumer The MessageConsumerInterface to pass messages to.
     * @return An HTTP2Stream from the stream pool, or @c nullptr if there was an error.
     */
    std::shared_ptr<HTTP2Stream> createPostStream(
        const std::string& url,
        const std::string& authToken,
        std::shared_ptr<avsCommon::avs::MessageRequest> request,
        std::shared_ptr<MessageConsumerInterface> messageConsumer);

    /**
     * Returns an HTTP2Stream back into the pool.
     * @param context Returns the given EventStream back into the pool.
     */
    void releaseStream(std::shared_ptr<HTTP2Stream> stream);

private:
    /**
     * Gets a stream from the stream pool  If the pool is empty, returns a new HTTP2Stream.
     *
     * @param messageConsumer The MessageConsumerInterface which should receive messages from AVS.
     * @return an HTTP2Stream from the pool, a new stream, or @c nullptr if there are too many active streams.
     */
    std::shared_ptr<HTTP2Stream> getStream(std::shared_ptr<MessageConsumerInterface> messageConsumer);

    /// A growing pool of EventStreams.
    std::vector<std::shared_ptr<HTTP2Stream>> m_pool;
    /// Mutex used to lock the EventStream pool.
    std::mutex m_mutex;
    /// The number of streams that have been acquired from the pool.
    int m_numAcquiredStreams;
    /// The maximum number of streams that can be active in the pool.
    const int m_maxStreams;
    /// The attachment manager.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> m_attachmentManager;
    /**
     * A static counter to ensure each newly acquired stream across all pools has a different ID.  The notion of a
     * stream ID is needed to provide a per-HTTP/2-stream context for any given attachment received from AVS.  AVS
     * can only guarantee that the identifying 'contentId' for an attachment is unique within a HTTP/2 stream,
     * hence this variable.
     * @note: These IDs are not to be confused with HTTP2's internal stream IDs.
     */
    static unsigned int m_nextStreamId;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2STREAMPOOL_H_
