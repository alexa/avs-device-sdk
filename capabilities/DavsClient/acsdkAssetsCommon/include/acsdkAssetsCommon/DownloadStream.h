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

#ifndef ACSDKASSETSCOMMON_DOWNLOADSTREAM_H_
#define ACSDKASSETSCOMMON_DOWNLOADSTREAM_H_

#include <fstream>
#include <memory>
#include <mutex>
#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/*
 * Consumer Producer queue with blocking wait and pop operation to store downloaded data chunks.
 * It comes with downloaded file size validation.
 */
class DownloadStream {
public:
    /**
     * Create download file object with expected size
     * @param path file path to write to
     * @param expectedSize expected download file size
     * @return downloadStream object or null if file path invalid
     */
    static std::shared_ptr<DownloadStream> create(const std::string& path, size_t expectedSize);

    virtual ~DownloadStream();

    /**
     * Write data chunk into the output stream
     * @param data pointer for data chunk
     * @param size number of bytes in the data chunk
     * @return true when successful, false for invalid argument or if accumulated size exceeds expectedSize
     */
    bool write(const char* data, size_t size);

    bool downloadSucceeded() const;

private:
    /**
     * Constructing a new object to hold download outputstream and its expected size
     * @param @outputStream ostream object where downloaded data will be written into
     * @param expectedSize expected download size.
     */
    explicit DownloadStream(const std::string& path, size_t expectedSize);

    /**
     * Whether the previous stream operation is good (no error)
     * @return true if no error, false otherwise
     */
    bool good() const;

    /// mutex to protect the member variables including m_downloadedSize
    mutable std::mutex m_mutex;

    /// output stream to write data to
    std::ofstream m_ostream;

    /// expected file size for the artifact to be downloaded
    size_t m_expectedSize;

    /// size downloaded so far
    size_t m_downloadedSize;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_DOWNLOADSTREAM_H_
