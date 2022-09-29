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

#if defined(FILE_SYSTEM_UTILS_ENABLED)

#include "AVSCommon/Utils/FileSystem/FileSystemUtils.h"
#include "AVSCommon/Utils/Logger/Logger.h"

#include <sys/stat.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>
#include <windows.h>
#include <io.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace filesystem {

/// String to identify log entries originating from this file.
#define TAG "FileSystemUtils"
/// Create a LogEntry using this file's TAG and the specified event std::string.
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

const Permissions DEFAULT_DIRECTORY_PERMISSIONS = OWNER_ALL | GROUP_READ | GROUP_EXEC;
const Permissions DEFAULT_FILE_PERMISSIONS = OWNER_READ | OWNER_WRITE | GROUP_READ;

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode)&S_IFMT) == S_IFREG)
#endif

#ifdef ACSDK_LOG_ENABLED
static std::string getStrError(int error) {
    static const size_t BUFFER_SIZE = 255;
    char buffer[BUFFER_SIZE + 1]{};
    auto ignore = strerror_s(buffer, BUFFER_SIZE, error);
    (void)ignore;  // unused since the type can differ depending on the gnu or posix
    return buffer;
}
#else
// Declaref function so it can be used in sizeof() expression when logging is disabled.
std::string getStrError(int);
#endif

static std::string getBackslashPath(std::string path) {
    replace(path.begin(), path.end(), '/', '\\');
    return path;
}

bool changePermissions(const std::string& path, Permissions perms) {
    int winPerms{};
    if (perms & OWNER_READ || perms & GROUP_READ || perms & OTHERS_READ) {
        winPerms |= _S_IREAD;
    }
    if (perms & OWNER_WRITE || perms & GROUP_WRITE || perms & OTHERS_WRITE) {
        winPerms |= _S_IWRITE;
    }

    if (_chmod(path.c_str(), winPerms) != 0) {
        ACSDK_ERROR(
            LX("changePermissions").m("Failed to change permission").d("path", path).d("error", getStrError(errno)));
        return false;
    }
    return true;
}

bool exists(const std::string& path) {
    struct stat fileStat {};
    return 0 == stat(path.c_str(), &fileStat);
}

std::string currentDirectory() {
    char cwd[MAX_PATH + 1]{};
    if (GetCurrentDirectoryA(MAX_PATH, cwd) == 0) {
        ACSDK_ERROR(LX("currentDirectory").m("Could not get current direction path"));
        return "";
    }

    ACSDK_DEBUG(LX("currentDirectory").d("path", cwd));
    return cwd;
}

bool makeDirectory(const std::string& inputPath, Permissions perms) {
    ACSDK_DEBUG7(LX("makeDirectory").d("path", inputPath));
    struct stat fileStat {};

    if (inputPath.empty()) {
        ACSDK_ERROR(LX("makeDirectory").m("Empty input path, unable to create directory"));
        return false;
    }

    if (stat(inputPath.c_str(), &fileStat) == 0) {
        if (S_ISDIR(fileStat.st_mode) == 0) {
            ACSDK_ERROR(LX("makeDirectory")
                            .m("Failed to create a directory, a file with the same name already exists")
                            .d("path", inputPath));
            return false;
        }
        if (!changePermissions(inputPath, perms)) {
            ACSDK_ERROR(LX("makeDirectory").m("Failed to change permission on existing directory"));
            return false;
        }
        return true;
    }

    std::string path = getBackslashPath(inputPath);
    if (path.find("\\..\\") != std::string::npos || path.find("\\.\\") != std::string::npos) {
        ACSDK_ERROR(
            LX("makeDirectory").m("Attempting to create filepath with \"\\..\\\" or \"\\.\\\"").d("path", path));
        return false;
    }

    auto iter = path.begin();
    while ((iter = find(iter + 1, path.end(), '\\')) != path.end()) {
        *iter = '\0';
        if (iter != path.begin() && *(iter - 1) == ':') {
            // ignore drive
        } else if (stat(path.c_str(), &fileStat) == 0) {
            // if path is NOT a directory, then we cannot progress
            if (!S_ISDIR(fileStat.st_mode)) {
                ACSDK_ERROR(LX("makeDirectory")
                                .m("Failed to create parent directory, a file with the same name already exists")
                                .d("path", path));
                return false;
            }
        } else if (!CreateDirectoryA(path.c_str(), nullptr)) {
            ACSDK_ERROR(LX("makeDirectory").m("Failed to create parent directory").d("path", path));
            return false;
        } else {
            ACSDK_DEBUG7(LX("makeDirectory").m("Created parent directory").d("path", path));
        }
        *iter = '\\';
    }

    if (!exists(path) && (!CreateDirectoryA(path.c_str(), nullptr) || !changePermissions(inputPath, perms))) {
        ACSDK_ERROR(LX("makeDirectory").m("Failed to create directory").d("path", path));
        return false;
    }

    ACSDK_INFO(LX("makeDirectory").m("Created final directory").d("path", path));
    return true;
}

std::vector<std::string> list(const std::string& path, const FileType type) {
    std::vector<std::string> result;
    WIN32_FIND_DATAA data{};
    auto handle = FindFirstFileA((path + "\\*").c_str(), &data);

    if (handle == INVALID_HANDLE_VALUE) {
        ACSDK_ERROR(LX("list").m("Could not open directory").d("path", path));
        return result;
    }

    do {
        std::string fileOrDirName = data.cFileName;
        // skip current and parent
        if (fileOrDirName == "." || fileOrDirName == "..") {
            continue;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (type == FileType::DIRECTORY || type == FileType::ALL) {
                result.push_back(fileOrDirName);
            }
        } else {
            if (type == FileType::REGULAR_FILE || type == FileType::ALL) {
                result.push_back(fileOrDirName);
            }
        }
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
    return result;
}

bool move(const std::string& source, const std::string& destination) {
    ACSDK_INFO(LX("move").d("source", source).d("destination", destination));
    auto result = std::rename(source.c_str(), destination.c_str());
    if (result != 0) {
        ACSDK_ERROR(LX("move").m("Move failed"));
        return false;
    }

    return true;
}

std::string basenameOf(const std::string& path) {
    char filename[_MAX_FNAME + 1]{};
    char ext[_MAX_EXT + 1]{};
    auto properPath = getBackslashPath(path);
    auto lastSlash = properPath.find_last_not_of('\\') + 1;
    _splitpath_s(properPath.substr(0, lastSlash).c_str(), nullptr, 0, nullptr, 0, filename, _MAX_FNAME, ext, _MAX_EXT);
    return std::string(filename) + ext;
}

std::string parentDirNameOf(const std::string& path) {
    char driveName[_MAX_DRIVE + 1]{};
    char dirName[_MAX_DIR + 1]{};
    auto properPath = getBackslashPath(path);
    auto lastSlash = properPath.find_last_not_of('\\') + 1;

    // if all we have is '\' or a series of '\\\', then what we have is the root folder only
    if (lastSlash == 0 && properPath[0] == '\\') {
        return "\\";
    }

    _splitpath_s(
        properPath.substr(0, lastSlash).c_str(), driveName, _MAX_DRIVE, dirName, _MAX_DIR, nullptr, 0, nullptr, 0);
    std::string driveStr = driveName;
    // if our directory is empty, then either return the drive only if provided (root folder) or current directory
    if (dirName[0] == '\0') {
        return driveStr.empty() ? ".\\" : driveStr + "\\";
    }

    return driveStr + dirName;
}

static bool removeDirectory(const std::string& path) {
    bool result = true;
    WIN32_FIND_DATAA data{};
    auto handle = FindFirstFileA((path + "\\*").c_str(), &data);

    if (handle == INVALID_HANDLE_VALUE) {
        ACSDK_ERROR(LX("removeDirectory").m("Could not open directory").d("path", path));
        return false;
    }

    do {
        std::string fileOrDirName = data.cFileName;
        std::string fullPath = path + "\\" + fileOrDirName;
        // skip current and parent
        if (fileOrDirName == "." || fileOrDirName == "..") {
            continue;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            result &= removeDirectory(fullPath);
        } else {
            result &= (0 == std::remove(fullPath.c_str()));
        }
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
    result &= static_cast<bool>(RemoveDirectoryA(path.c_str()));
    return result;
}

bool removeAll(const std::string& path) {
    ACSDK_INFO(LX("removeAll").d("path", path));
    struct stat pathStat {};
    if (stat(path.c_str(), &pathStat) != 0) {
        ACSDK_DEBUG7(LX("removeAll").m("Path does not exists").d("path", path));
        return true;
    }

    if (!S_ISDIR(pathStat.st_mode)) {
        if (std::remove(path.data()) != 0) {
            ACSDK_ERROR(LX("removeAll").m("Failed to delete file").d("path", path));
            return false;
        }
        return true;
    }

    return removeDirectory(path);
}

static size_t sizeOfDirectory(const std::string& path) {
    size_t size{};
    WIN32_FIND_DATAA data{};
    auto handle = FindFirstFileA((path + "\\*").c_str(), &data);

    if (handle == INVALID_HANDLE_VALUE) {
        ACSDK_ERROR(LX("sizeOfDirectory").m("Could not open directory path").d("path", path));
        return 0;
    }

    do {
        std::string fileOrDirName = data.cFileName;
        std::string fullPath = path + "\\" + fileOrDirName;
        // skip current and parent
        if (fileOrDirName == "." || fileOrDirName == "..") {
            continue;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            size += sizeOfDirectory(fullPath);
        } else {
            size += data.nFileSizeHigh * MAXDWORD + data.nFileSizeLow;
        }
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
    return size;
}

size_t sizeOf(const std::string& path) {
    struct stat pathStat {};
    if (stat(path.c_str(), &pathStat) != 0) {
        ACSDK_ERROR(LX("sizeOf").m("Path does not exists").d("path", path));
        return 0;
    }

    if (S_ISDIR(pathStat.st_mode)) {
        return sizeOfDirectory(path);
    }

    ACSDK_DEBUG(LX("sizeOf").d("path", path).d("bytes", pathStat.st_size));
    return static_cast<size_t>(pathStat.st_size);
}

size_t availableSpace(const std::string& path) {
    ULARGE_INTEGER i64FreeBytesToCaller;
    if (!GetDiskFreeSpaceExA(path.c_str(), &i64FreeBytesToCaller, nullptr, nullptr)) {
        ACSDK_ERROR(LX("availableSpace").m("Failed to get free space from system").d("path", path));
        return 0UL;
    }
    return static_cast<size_t>(i64FreeBytesToCaller.QuadPart);
}

bool pathContainsPrefix(const std::string& path, const std::string& prefix) {
    char resolvedPath[MAX_PATH + 1]{};
    char resolvedPrefix[MAX_PATH + 1]{};
    if (GetFullPathNameA(path.c_str(), MAX_PATH, resolvedPath, nullptr) == 0) {
        ACSDK_ERROR(LX("pathContainsPrefix").m("Unable to resolve path").d("path", path));
        return false;
    }
    if (GetFullPathNameA(prefix.c_str(), MAX_PATH, resolvedPrefix, nullptr) == 0) {
        ACSDK_ERROR(LX("pathContainsPrefix").m("Unable to resolve prefix").d("prefix", prefix));
        return false;
    }

    // assert resolved path is a prefix
    return strncmp(resolvedPrefix, resolvedPath, strnlen(resolvedPrefix, MAX_PATH)) == 0;
}

}  // namespace filesystem
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // FILE_SYSTEM_UTILS_ENABLED
