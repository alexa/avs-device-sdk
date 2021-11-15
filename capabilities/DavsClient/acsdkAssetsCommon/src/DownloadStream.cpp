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

#include "acsdkAssetsCommon/DownloadStream.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;

/// String to identify log entries originating from this file.
static const std::string TAG{"DownloadStream"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<DownloadStream> DownloadStream::create(const string& path, size_t expectedSize) {
    shared_ptr<DownloadStream> downloadStream(new DownloadStream(path, expectedSize));
    return (downloadStream->good()) ? downloadStream : nullptr;
}

DownloadStream::DownloadStream(const std::string& path, size_t expectedSize) :
        m_ostream(path),
        m_expectedSize(expectedSize),
        m_downloadedSize(0) {
}

bool DownloadStream::good() const {
    unique_lock<mutex> lock(m_mutex);
    return m_ostream.good();
}

DownloadStream::~DownloadStream() {
    unique_lock<mutex> lock(m_mutex);
    m_ostream.close();
}

bool DownloadStream::write(const char* data, size_t size) {
    if (data == nullptr) {
        ACSDK_ERROR(LX("write").m("Cannot write bytes from nullptr").d("number of bytes", size));
        return false;
    }
    unique_lock<mutex> lock(m_mutex);
    if (m_expectedSize != 0 && (m_downloadedSize > m_expectedSize || size > m_expectedSize - m_downloadedSize)) {
        ACSDK_ERROR(LX("write").m("Downloaded size exceeds expected size").d("expected size", m_expectedSize));
        return false;
    }
    m_ostream.write(data, size);
    auto ret = m_ostream.good();
    if (ret) {
        m_downloadedSize += size;
    }
    return ret;
}

bool DownloadStream::downloadSucceeded() const {
    unique_lock<mutex> lock(m_mutex);
    if (m_expectedSize != 0 && m_downloadedSize != m_expectedSize) {
        ACSDK_ERROR(LX("downloadSucceeded")
                            .m("Downloaded size mismatch expected size")
                            .d("downloaded size", m_downloadedSize)
                            .d("expected size", m_expectedSize));
        return false;
    }
    return true;
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
