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

#ifndef ACSDKASSETSCOMMON_ARCHIVEWRAPPER_H_
#define ACSDKASSETSCOMMON_ARCHIVEWRAPPER_H_

#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>

struct archive;

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/**
 * Wraps the libarchive library as libarchive is not thread-safe.
 * One one libarchive operation could happen at a given time.
 */
class ArchiveWrapper {
public:
    // Destructor
    virtual ~ArchiveWrapper() = default;

    /**
     * Return an instance of this class. There should be only one instance of this class.
     * @return an instance of this class, create one if it did not exist.
     */
    static std::shared_ptr<ArchiveWrapper> getInstance();

    /**
     * Get the total size of the contents of an archive when uncompressed.
     * NOT the size of the archive on disk.
     *
     * @param fileName of the archive to get the size of.
     * @return size of the given archive, or SIZE_MAX if an error occurs.
     */
    size_t sizeOfArchive(const std::string& fileName);

    /**
     * Uncompresses a given tar.gz or zip or other compression formats supported by libarchive into the destination
     * folder
     * @param fileName name of compressed file
     * @param destFolder directory to place on. Must end in /
     * @param directoryPermission OPTIONAL, permissions to use for the unpacked directories
     * @param filePermission OPTIONAL, permissions to use for the unpacked files
     */
    bool unpack(
            const std::string& fileName,
            const std::string& destFolder,
            alexaClientSDK::avsCommon::utils::filesystem::Permissions directoryPermission =
                    alexaClientSDK::avsCommon::utils::filesystem::DEFAULT_DIRECTORY_PERMISSIONS,
            alexaClientSDK::avsCommon::utils::filesystem::Permissions filePermission =
                    alexaClientSDK::avsCommon::utils::filesystem::DEFAULT_FILE_PERMISSIONS);

    /**
     * Uncompresses a given tar.gz or zip or other compression formats supported by libarchive into the destination
     * folder
     * @param readArchive archive object that read archived data
     * @param writeArchive archive object that write unpacked data
     * @param destFolder directory to place unpacked data in. Must not end in /
     * @param directoryPermission OPTIONAL, permissions to use for the unpacked directories
     * @param filePermission OPTIONAL, permissions to use for the unpacked files
     */
    bool unpack(
            struct archive* reader,
            struct archive* writer,
            const std::string& destFolder,
            alexaClientSDK::avsCommon::utils::filesystem::Permissions directoryPermission =
                    alexaClientSDK::avsCommon::utils::filesystem::DEFAULT_DIRECTORY_PERMISSIONS,
            alexaClientSDK::avsCommon::utils::filesystem::Permissions filePermission =
                    alexaClientSDK::avsCommon::utils::filesystem::DEFAULT_FILE_PERMISSIONS);

private:
    /// private constructor
    ArchiveWrapper() = default;

    /// instance of this object
    static std::shared_ptr<ArchiveWrapper> m_instance;

    /// mutex to serialize API calls as libArchive is not thread-safe
    std::mutex m_mutex;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_ARCHIVEWRAPPER_H_
