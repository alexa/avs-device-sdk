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

#include "acsdkAssetsCommon/DownloadChunkQueue.h"

#include <AVSCommon/Utils/Logger/Logger.h>

#include <algorithm>

#include "acsdkAssetsCommon/AmdMetricWrapper.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace chrono;

static const auto s_metrics = AmdMetricsWrapper::creator("DownloadChunkQueue");
static const auto DOWNLOAD_CHUNK_MAX_WAIT_TIME = seconds(60);
static const auto DOWNLOAD_REPORT_MINIMAL_BYTES = 100000;

/// String to identify log entries originating from this file.
static const std::string TAG{"DownloadChunkQueue"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

DownloadChunkQueue::DownloadChunkQueue(size_t expectedSize) :
        m_expectedSize(expectedSize),
        m_downloadedSize(0),
        m_downloadStatus(StreamingStatus::INPROGRESS),
        m_unpackStatus(StreamingStatus::INPROGRESS),
        m_maxQueueSizeReached(0),
        m_bytesToReport(0),
        m_reportIncrement(
                m_expectedSize ? max<size_t>(DOWNLOAD_REPORT_MINIMAL_BYTES, m_expectedSize / 8)
                               : DOWNLOAD_REPORT_MINIMAL_BYTES) {
    ACSDK_INFO(LX("DownloadChunkQueue").m("Created DownloadChunkQueue").d("expectedSize", expectedSize));
}

DownloadChunkQueue::~DownloadChunkQueue() {
    unique_lock<mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
}

bool DownloadChunkQueue::push(char* data, size_t size) {
    auto retValue = false;

    {
        unique_lock<mutex> lock(m_mutex);

        if (m_unpackStatus != StreamingStatus::INPROGRESS) {
            ACSDK_ERROR(LX("push").m("push failed, unpack no longer in progress").d("Number of bytes", size));
            return false;
        }
        switch (m_downloadStatus) {
            case StreamingStatus::INPROGRESS:
                if (data == nullptr || size == 0) {
                    ACSDK_ERROR(LX("push").m("Invalid push request").d("Number of bytes", size));
                    m_downloadStatus = StreamingStatus::ABORTED;
                } else {
                    m_downloadedSize += size;
                    if (isSizeCheckingEnabled() && m_downloadedSize > m_expectedSize) {
                        ACSDK_ERROR(LX("push")
                                            .m("Downloaded size exceeds expected size")
                                            .d("Downloaded size", m_downloadedSize)
                                            .d("Expected Size", m_expectedSize));
                        m_downloadStatus = StreamingStatus::ABORTED;
                    } else {
                        auto dataChunk = make_shared<DataChunk>(data, size);
                        m_queue.push(dataChunk);
                        auto currentQueueSize = m_queue.size();
                        if (currentQueueSize > m_maxQueueSizeReached) {
                            m_maxQueueSizeReached = currentQueueSize;
                        }
                        if (m_downloadedSize > m_bytesToReport) {
                            if (m_expectedSize > 0) {
                                ACSDK_INFO(LX("push")
                                                   .m("Pushed bytes to queue")
                                                   .d("Downloaded size", m_downloadedSize)
                                                   .d("Expected Size", m_expectedSize)
                                                   .d("current queue size", currentQueueSize));
                            } else {
                                ACSDK_INFO(LX("push")
                                                   .m("Pushed bytes to queue")
                                                   .d("Downloaded size", m_downloadedSize)
                                                   .d("current queue size", currentQueueSize));
                            }
                            m_bytesToReport = m_downloadedSize + m_reportIncrement;
                        }
                    }
                }
                break;
            case StreamingStatus::COMPLETED:
                ACSDK_ERROR(LX("push")
                                    .m("Invalid push of bytes after download has been completed")
                                    .d("number of bytes", size));
                m_downloadStatus = StreamingStatus::ABORTED;
                break;
            case StreamingStatus::ABORTED:
                ACSDK_ERROR(LX("push")
                                    .m("Invalid push of bytes after download has been aborted")
                                    .d("number of bytes", size));
                break;
        }
        retValue = (m_downloadStatus == StreamingStatus::INPROGRESS);
    }
    m_cond.notify_all();
    return retValue;
}

bool DownloadChunkQueue::pushComplete(bool succeeded) {
    auto retValue = false;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_unpackStatus != StreamingStatus::INPROGRESS) {
            ACSDK_ERROR(LX("pushComplete").m("pushComplete - unpack no longer in progress"));
            return false;
        }
        switch (m_downloadStatus) {
            case StreamingStatus::COMPLETED:
                ACSDK_WARN(LX("pushComplete").m("pushComplete has already been invoked"));
                // although not supposed to happen (calling pushComplete multiple times,
                // it should cause no harm. Continue on inprogress case, no need to break here
            case StreamingStatus::INPROGRESS:
                if (!succeeded) {
                    m_downloadStatus = StreamingStatus::ABORTED;
                } else {
                    if (isSizeCheckingEnabled() && m_downloadedSize != m_expectedSize) {
                        ACSDK_ERROR(LX("pushComplete")
                                            .m("download size mismatch expected size")
                                            .d("downoload size", m_downloadedSize)
                                            .d("exepected size", m_expectedSize));
                        m_downloadStatus = StreamingStatus::ABORTED;
                    } else {
                        ACSDK_INFO(LX("pushComplete")
                                           .d("Pushed bytes", m_downloadedSize)
                                           .d("m_maxQueueSizeReached ", m_maxQueueSizeReached));
                        m_downloadStatus = StreamingStatus::COMPLETED;
                    }
                }
                break;
            case StreamingStatus::ABORTED:
                // once error is detected, it will remain errored out
                break;
        }  // switch(m_downloadStatus)
        retValue = (m_downloadStatus == StreamingStatus::COMPLETED);
    }  // lock
    m_cond.notify_all();
    return retValue;
}

shared_ptr<DataChunk> DownloadChunkQueue::waitAndPop() {
    shared_ptr<DataChunk> dataChunk;
    unique_lock<mutex> lock(m_mutex);
    switch (m_unpackStatus) {
        case StreamingStatus::COMPLETED:
            ACSDK_ERROR(LX("waitAndPop").m("waitAndPop invoked after unpack Completed"));
            break;
        case StreamingStatus::ABORTED:
            ACSDK_ERROR(LX("waitAndPop").m("waitAndPop invoked after unpack Aborted"));
            break;
        case StreamingStatus::INPROGRESS:
            while (m_downloadStatus == StreamingStatus::INPROGRESS && m_queue.empty()) {
                m_cond.wait_for(lock, DOWNLOAD_CHUNK_MAX_WAIT_TIME);
            }
            switch (m_downloadStatus) {
                case StreamingStatus::ABORTED:
                    ACSDK_ERROR(LX("waitAndPop").m("waitAndPop failed, download Aborted"));
                    break;
                case StreamingStatus::COMPLETED:
                    if (!m_queue.empty()) {
                        dataChunk = m_queue.front();
                        m_queue.pop();
                    } else {
                        ACSDK_INFO(LX("waitAndPop").m("waitAndPop done, no more chunks"));
                    }
                    break;
                case StreamingStatus::INPROGRESS:
                    if (!m_queue.empty()) {
                        dataChunk = m_queue.front();
                        m_queue.pop();
                    } else {
                        ACSDK_ERROR(LX("waitAndPop").m("waitAndPop timedout"));
                    }
                    break;
            }  // switch (m_downloadStatus)
            break;
    }  // switch (m_unpackStatus)
    // We need to keep a reference to dataChunk object so when dataChunk->data is passed to unpack callback
    // the data is still valid. When pop is called again, the previous dataChunk has already finished processing
    // by unpack function
    m_activeChunk = dataChunk;
    return dataChunk;
}

bool DownloadChunkQueue::popComplete(bool succeeded) {
    unique_lock<mutex> lock(m_mutex);
    switch (m_unpackStatus) {
        case StreamingStatus::ABORTED:
            ACSDK_ERROR(LX("popComplete").m("popComplete invoked after unpack Aborted"));
            break;
        case StreamingStatus::COMPLETED:
            if (!succeeded) {
                ACSDK_ERROR(LX("popComplete").m("popComplete initiated abort after unpack Completed"));
                m_unpackStatus = StreamingStatus::ABORTED;
            } else {
                ACSDK_WARN(LX("popComplete").m("popComplete initiated abort after unpack aborted"));
            }
            break;
        case StreamingStatus::INPROGRESS:
            if (!succeeded) {
                ACSDK_ERROR(LX("popComplete").m("popComplete initiated abort while unpack in progress"));
                m_unpackStatus = StreamingStatus::ABORTED;
            } else {
                while (m_downloadStatus == StreamingStatus::INPROGRESS && m_queue.empty()) {
                    ACSDK_INFO(LX("popComplete").m("popComplete wait for download pushComplete event"));
                    m_cond.wait_for(lock, DOWNLOAD_CHUNK_MAX_WAIT_TIME);
                }
                switch (m_downloadStatus) {
                    case StreamingStatus::ABORTED:
                        ACSDK_ERROR(LX("popComplete").m("popComplete initiated while download aborted"));
                        m_unpackStatus = StreamingStatus::ABORTED;
                        break;
                    case StreamingStatus::COMPLETED:
                        if (!m_queue.empty()) {
                            ACSDK_ERROR(LX("popComplete")
                                                .m("popComplete initiated while download chucks left")
                                                .d("download chunks left", m_queue.size()));
                            m_unpackStatus = StreamingStatus::ABORTED;
                        } else {
                            ACSDK_INFO(LX("popComplete").m("popComplete initiated while download completed"));
                            m_unpackStatus = StreamingStatus::COMPLETED;
                        }
                        break;
                    case StreamingStatus::INPROGRESS:
                        ACSDK_ERROR(LX("popComplete")
                                            .m("popComplete initiated while download in progress")
                                            .d("chucks in queue", m_queue.size()));
                        m_unpackStatus = StreamingStatus::ABORTED;
                        break;
                }  // switch (m_downloadStatus)
            }
            break;  // case StreamingStatus::INPROGRESS:
    }               // switch (m_unpackStatus)
    return (m_unpackStatus == StreamingStatus::COMPLETED);
}

size_t DownloadChunkQueue::size() {
    unique_lock<mutex> lock(m_mutex);
    return m_queue.size();
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
