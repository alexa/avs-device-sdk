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

#ifndef ACSDKASSETSCOMMON_DOWNLOADCHUNKQUEUE_H_
#define ACSDKASSETSCOMMON_DOWNLOADCHUNKQUEUE_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "DataChunk.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/**
 * Status of streaming download/unpack
 */
enum class StreamingStatus { INPROGRESS, COMPLETED, ABORTED };

/*
 * Consumer Producer queue with blocking wait and pop operation to store downloaded data chunks.
 * It comes with downloaded file size validation.
 */
class DownloadChunkQueue {
public:
    /**
     * Constructing a new queue to hold downloaded data chunks
     * @param expectedSize expected download size. Pushing more or less data before completion signals error
     *                     unless the user has signaled no size check with the expected size of 0.
     */
    explicit DownloadChunkQueue(size_t expectedSize);

    virtual ~DownloadChunkQueue();

    /**
     * Returning number of data chunks in the queue.
     * @return number of data chunks in the queue.
     */
    size_t size();

    /**
     * Producer pushes new data chunk into download queue
     * @param data pointer for data chunk
     * @param size number of bytes in the data chunk
     * @return true when successful, false for invalid argument or if accumulated size exceeds expectedSize
     */
    bool push(char* data, size_t size);

    /**
     * Producer signals push completion
     * @param succeeded whether download succeeded
     * @return whether total downloaded size matches the expected size unless aborted if size check isn't turned off
     */
    bool pushComplete(bool succeeded);

    /**
     * Blocking wait and get the next data chunk from queue.
     * @return next data chunk from the front of the queue, or nullptr if error has been detected or no more data.
     * The last waitAndPop should be followed by popComplte()
     */
    std::shared_ptr<DataChunk> waitAndPop();

    /**
     * Consumer signals the completion of reading from queue. Will wait until producer pushComplete
     * or return false if data chunks still available in queue.
     * @param succeeded whether the completion is triggered by an error/succes condition
     * @return false if there're still remaining chunks in the queue or download has failed
     */
    bool popComplete(bool succeeded);

private:
    /**
     * @return true if size checking is enabled (that is expected size is not 0)
     */
    inline bool isSizeCheckingEnabled() const {
        return m_expectedSize > 0;
    }

private:
    /// condition variable to signal blocking pop function new chunks available
    std::condition_variable m_cond;

    /// mutex to protect the member variables including m_queue
    std::mutex m_mutex;

    /// queue holding downloaded data chunks
    std::queue<std::shared_ptr<DataChunk>> m_queue;

    /// expected file size for the artifact to be downloaded
    size_t m_expectedSize;

    /// size downloaded so far (pushed into queue)
    size_t m_downloadedSize;

    /// status of producer (curl download)
    StreamingStatus m_downloadStatus;

    /// status of consumer (archive unpack)
    StreamingStatus m_unpackStatus;

    /// current active (unpacking) data chunk that has been popped from queue
    /// We keep it here so raw data pointer would still be valid after after it has been passed to libarchive
    std::shared_ptr<DataChunk> m_activeChunk;

    /// maximum queue size reached, for analysis and future optimization purpose
    size_t m_maxQueueSizeReached;

    /// Next download benchmark to log
    size_t m_bytesToReport;

    /// download increment to report
    size_t m_reportIncrement;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_DOWNLOADCHUNKQUEUE_H_
