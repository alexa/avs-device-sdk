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

#include "acsdkAssetsCommon/ArchiveWrapper.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <archive.h>
#include <archive_entry.h>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace alexaClientSDK::avsCommon::utils;

static const std::string TAG{"ArchiveWrapper"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<ArchiveWrapper> ArchiveWrapper::m_instance = nullptr;
static constexpr size_t SIXTY_FOUR_MEGABYTES = 64 * 1024 * 1024;

shared_ptr<ArchiveWrapper> ArchiveWrapper::getInstance() {
    static mutex s_mutex;
    unique_lock<mutex> lock(s_mutex);

    if (ArchiveWrapper::m_instance == nullptr) {
        ArchiveWrapper::m_instance.reset(new ArchiveWrapper());
    }
    return ArchiveWrapper::m_instance;
}

size_t ArchiveWrapper::sizeOfArchive(const std::string& fileName) {
    auto readArchive = unique_ptr<archive, decltype(&archive_read_free)>(archive_read_new(), archive_read_free);

    // Other archive formats are supported as well: see '*_all' in libarchive
    archive_read_support_format_all(readArchive.get());

    // Other filters are supported as well: see '*_all' in libarchive
    archive_read_support_filter_all(readArchive.get());

    // Open a the archive file with the maximum buffer size
    auto archiveStatus = archive_read_open_filename(readArchive.get(), fileName.data(), 10240);
    if (archiveStatus != ARCHIVE_OK) {
        ACSDK_ERROR(LX("sizeOfArchive").m("Failed to open file").d("error", archive_error_string(readArchive.get())));
        return SIZE_MAX;
    }

    size_t archiveSize = 0;

    struct archive_entry* entry;
    while (true) {
        archiveStatus = archive_read_next_header(readArchive.get(), &entry);
        if (archiveStatus == ARCHIVE_EOF) {
            break;
        }

        if (archiveStatus != ARCHIVE_OK) {
            ACSDK_ERROR(LX("sizeOfArchive")
                                .m("Failed to read next header")
                                .d("error", archive_error_string(readArchive.get())));
            archiveSize = SIZE_MAX;
            break;
        }

        int64_t archiveEntrySize = archive_entry_size(entry);
        if (archiveEntrySize <= 0) {
            ACSDK_ERROR(LX("sizeOfArchive").m("Archive entry has invalid").d("size", archiveEntrySize));
            archiveSize = SIZE_MAX;
            break;
        }

        archiveSize += archiveEntrySize;
    }

    archive_read_close(readArchive.get());
    return archiveSize;
}

static int copyData(struct archive* readArchive, struct archive* writeArchive) {
    const void* buff;
    size_t size;
    int64_t offset;
    size_t total_bytes_written = 0;

    while (true) {
        auto result = archive_read_data_block(readArchive, &buff, &size, &offset);
        total_bytes_written += size;

        if (total_bytes_written > SIXTY_FOUR_MEGABYTES) {
            return ARCHIVE_FAILED;
        }

        if (result == ARCHIVE_EOF) return (ARCHIVE_OK);
        if (result != ARCHIVE_OK) return (result);
        result = static_cast<int>(archive_write_data_block(writeArchive, buff, size, offset));
        if (result != ARCHIVE_OK) {
            return (result);
        }
    }
}

static bool unpackLocked(
        struct archive* reader,
        struct archive* writer,
        const string& destFolder,
        filesystem::Permissions directoryPermission,
        filesystem::Permissions filePermission) {
    vector<string> writtenFilesList;
    auto allFilesWrittenSuccessfully = true;

    int archiveStatus = ARCHIVE_EOF;

    ACSDK_INFO(LX("unpackLocked").m("start unpacking archive").d("destination", destFolder));
    struct archive_entry* entry;
    while (true) {
        archiveStatus = archive_read_next_header(reader, &entry);
        if (archiveStatus == ARCHIVE_EOF) {
            break;
        }

        if (archiveStatus != ARCHIVE_OK) {
            ACSDK_ERROR(LX("unpackLocked").m("Failed to read next header").d("error", archive_error_string(reader)));
            allFilesWrittenSuccessfully = false;
            break;
        }

        // write the file from archive into the destination folder with the same file name
        const string fullOutputPath = destFolder + "/" + archive_entry_pathname(entry);
        if (!filesystem::pathContainsPrefix(fullOutputPath, destFolder)) {
            // path wasn't under destFolder, something isn't right
            ACSDK_ERROR(LX("unpackLocked").m("Unable to write to destination").d("path", fullOutputPath));
            allFilesWrittenSuccessfully = false;
            break;
        }

        archive_entry_set_pathname(entry, fullOutputPath.c_str());
        writtenFilesList.push_back(fullOutputPath);

        // write this single file
        archiveStatus = archive_write_header(writer, entry);
        if (archiveStatus != ARCHIVE_OK) {
            ACSDK_ERROR(LX("unpackLocked").m("Failed to write header").d("error", archive_error_string(writer)));
            allFilesWrittenSuccessfully = false;
            break;
        }

        copyData(reader, writer);
        archiveStatus = archive_write_finish_entry(writer);
        struct stat fileStat {};
        auto rc = stat(fullOutputPath.data(), &fileStat);
        if (archiveStatus != ARCHIVE_OK || rc != 0) {
            ACSDK_ERROR(LX("unpackLocked")
                                .m("Failed to close file we just wrote")
                                .d("error", archive_error_string(writer)));
            allFilesWrittenSuccessfully = false;
            break;
        }

        auto perm = (fileStat.st_mode & S_IFMT) == S_IFDIR ? directoryPermission : filePermission;
        filesystem::changePermissions(fullOutputPath, perm);
    }

    if (archive_read_close(reader) != ARCHIVE_OK) {
        allFilesWrittenSuccessfully = false;
    }
    if (archive_write_close(writer) != ARCHIVE_OK) {
        allFilesWrittenSuccessfully = false;
    }

    // clean up written files if a file is corrupt
    if (!allFilesWrittenSuccessfully) {
        ACSDK_WARN(LX("unpackLocked").m("Failed to write files, cleaning up"));
        for (const auto& filePath : writtenFilesList) {
            filesystem::removeAll(filePath);
        }
    }

    return allFilesWrittenSuccessfully;
}

bool ArchiveWrapper::unpack(
        const string& fileName,
        const string& destFolder,
        const filesystem::Permissions directoryPermission,
        const filesystem::Permissions filePermission) {
    unique_lock<mutex> lock(m_mutex);
    ACSDK_INFO(LX("unpack").m("start unpacking").d("source", fileName).d("destination", destFolder));

    auto readArchive = unique_ptr<archive, decltype(&archive_read_free)>(archive_read_new(), archive_read_free);
    auto writeArchive =
            unique_ptr<archive, decltype(&archive_write_free)>(archive_write_disk_new(), archive_write_free);

    // Other archive formats are supported as well: see '*_all' in libarchive
    archive_read_support_format_all(readArchive.get());

    // Other filters are supported as well: see '*_all' in libarchive
    archive_read_support_filter_all(readArchive.get());

    // Open a the archive file with the maximum buffer size
    if (ARCHIVE_OK != archive_read_open_filename(readArchive.get(), fileName.data(), 10240)) {
        ACSDK_ERROR(LX("unpack").m("Failed to open filename").d("error", archive_error_string(readArchive.get())));
        return false;
    }
    return unpackLocked(readArchive.get(), writeArchive.get(), destFolder, directoryPermission, filePermission);
}

bool ArchiveWrapper::unpack(
        struct archive* reader,
        struct archive* writer,
        const string& destFolder,
        const filesystem::Permissions directoryPermission,
        const filesystem::Permissions filePermission) {
    if (reader == nullptr || writer == nullptr) {
        ACSDK_ERROR(LX("unpack").m("Invalid archive reader/writer"));
        return false;
    }

    unique_lock<mutex> lock(m_mutex);
    return unpackLocked(reader, writer, destFolder, directoryPermission, filePermission);
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
