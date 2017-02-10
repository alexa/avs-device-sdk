/*
 * HTTP2StreamPool.h
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

#ifndef ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_STREAM_POOL_H_
#define ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_STREAM_POOL_H_

#include "ACL/Transport/HTTP2Stream.h"
#include <vector>
#include <mutex>
#include <atomic>

namespace alexaClientSDK {
namespace acl {

class HTTP2StreamPool {
public:
    /**
     * Default constructor
     *
     * @params maxStreams The maximum number of streams that can be active
     */
    HTTP2StreamPool(const int maxStreams);

    /**
     * Grabs an HTTP2Stream from the pool and configures it to be an HTTP GET.
     *
     * @param transport The HTTP2Transport this stream reports back to. The HTTP2Transport will own the
     * underlying object and be responsible for cleaning it up.
     * @param url The request URL.
     * @param authToken The LWA token to supply.
     * @return An HTTP2Stream from the stream pool, or @c nullptr if there was an error.
     */
    std::shared_ptr<HTTP2Stream> createGetStream(HTTP2Transport *transport, const std::string& url,
                                        const std::string& authToken);

    /**
     * Grabs an HTTP2Stream from the pool and configures it to be an HTTP POST.
     *
     * @param transport The HTTP2Transport this stream reports back to.
     * @param url The request URL.
     * @param authToken The LWA token to supply.
     * @param request The message request.
     * @return An HTTP2Stream from the stream pool, or @c nullptr if there was an error.
     */
    std::shared_ptr<HTTP2Stream> createPostStream(HTTP2Transport *transport, const std::string& url,
                                        const std::string& authToken, std::shared_ptr<MessageRequest> request);

    /**
     * Returns an HTTP2Stream back into the pool.
     * @param context Returns the given EventStream back into the pool.
     */
    void releaseStream(std::shared_ptr<HTTP2Stream> stream);
private:
    /**
     * Gets a stream from the stream pool  If the pool is empty, returns a new HTTP2Stream.
     * @returns an HTTP2Stream from the pool, a new stream, or @c nullptr if there are too many active streams.
     */
    std::shared_ptr<HTTP2Stream> getStream();

    /// A growing pool of EventStreams.
    std::vector<std::shared_ptr<HTTP2Stream>> m_pool;
    /// Mutex used to lock the EventStream pool.
    std::mutex m_mutex;
    /// The number of streams that have been released from the pool.
    std::atomic<int> m_numRemovedStreams;
    /// The maximum number of streams that can be active in the pool.
    const int m_maxStreams;
};

} // acl
} // alexaClientSDK

#endif // ALEXACLIENTSDK_ACL_INCLUDE_ACL_TRANSPORT_STREAM_POOL_H_
