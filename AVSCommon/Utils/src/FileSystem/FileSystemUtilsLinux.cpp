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

#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <stack>
#include <vector>

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

/**
 * Simple locker class that protects the processes umask using a global mutex
 */
class UmaskLocker {
public:
    explicit UmaskLocker(mode_t newMode) {
        s_umaskMutex.lock();
        m_storedPermission = umask(newMode);  // NOLINT(cppcoreguidelines-prefer-member-initializer) set after lock
    }

    ~UmaskLocker() {
        umask(m_storedPermission);
        s_umaskMutex.unlock();
    }

private:
    static std::mutex s_umaskMutex;

    mode_t m_storedPermission;
};

std::mutex UmaskLocker::s_umaskMutex;

#ifdef ACSDK_LOG_ENABLED
static std::string getStrError(int error) {
    static const size_t BUFFER_SIZE = 255;
    char buffer[BUFFER_SIZE + 1]{};
    auto ignore = strerror_r(error, buffer, BUFFER_SIZE);
    (void)ignore;  // unused since the type can differ depending on the gnu or posix
    return buffer;
}
#else
// Declaref function so it can be used in sizeof() expression when logging is disabled.
std::string getStrError(int);
#endif

bool changePermissions(const std::string& path, Permissions perms) {
    if (chmod(path.c_str(), perms) != 0) {
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
    char cwd[PATH_MAX + 1]{};
    if (getcwd(cwd, PATH_MAX) == nullptr) {
        ACSDK_ERROR(LX("currentDirectory").m("Failed to get current directory path"));
        return "";
    }

    ACSDK_DEBUG(LX("currentDirectory").d("path", cwd));
    return cwd;
}

bool makeDirectory(const std::string& inputPath, Permissions perms) {
    ACSDK_DEBUG7(LX("makeDirectory").d("path", inputPath));
    struct stat fileStat {};

    if (inputPath.empty()) {
        ACSDK_ERROR(LX("makeDirectory").m("Empty input path, unable to create directory").d("path", inputPath));
        return false;
    }

    if (lstat(inputPath.c_str(), &fileStat) == 0) {
        if (!S_ISDIR(fileStat.st_mode)) {
            ACSDK_ERROR(LX("makeDirectory")
                            .m("Failed to create directory, a file with the same name already exists")
                            .d("path", inputPath));
            return false;
        }
        if (!changePermissions(inputPath, perms)) {
            ACSDK_ERROR(LX("makeDirectory").m("Failed to change permission on existing directory"));
            return false;
        }
        return true;
    }

    if (inputPath.find("/../") != std::string::npos || inputPath.find("/./") != std::string::npos) {
        ACSDK_ERROR(
            LX("makeDirectory").m("Attempting to create filepath with \"/../\" or \"/./\"%s").d("path", inputPath));
        return false;
    }

    std::string path = inputPath;
    const auto umaskLocker = UmaskLocker(0);
    auto iter = path.begin();
    while ((iter = find(iter + 1, path.end(), '/')) != path.end()) {
        *iter = '\0';
        // if path already exists
        if (stat(path.c_str(), &fileStat) == 0) {
            // if path is NOT a directory, then we cannot progress
            if (!S_ISDIR(fileStat.st_mode)) {
                ACSDK_ERROR(LX("makeDirectory")
                                .m("Failed to create parent directory, a file with the same name already exists")
                                .d("path", path));
                return false;
            }
        } else if (mkdir(path.c_str(), perms) != 0 && errno != EEXIST) {
            ACSDK_ERROR(LX("makeDirectory")
                            .m("Failed to create parent directory")
                            .d("path", path)
                            .d("error", getStrError(errno)));
            return false;
        } else {
            ACSDK_DEBUG7(LX("makeDirectory").m("Created parent directory").d("path", path));
        }
        *iter = '/';
    }

    if (!exists(path) && mkdir(path.c_str(), perms) != 0 && errno != EEXIST) {
        ACSDK_ERROR(LX("makeDirectory").m("Failed to create directory").d("path", path).d("error", getStrError(errno)));
        return false;
    }

    ACSDK_INFO(LX("makeDirectory").m("Created final directory").d("path", path));
    return true;
}

std::vector<std::string> list(const std::string& path, const FileType type) {
    static std::mutex listMutex;
    std::lock_guard<std::mutex> lock(listMutex);
    std::vector<std::string> result;

    auto dirPtr = std::unique_ptr<DIR, int (*)(DIR*)>(opendir(path.c_str()), &closedir);
    while (dirPtr != nullptr) {
        auto dp = readdir(dirPtr.get());  // NOLINT(concurrency-mt-unsafe) protected by static mutex
        if (dp == nullptr) {
            break;
        }
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        FileType currentType;
        if (dp->d_type == DT_DIR) {
            currentType = FileType::DIRECTORY;
        } else if (dp->d_type == DT_REG) {
            currentType = FileType::REGULAR_FILE;
        } else {
            // only list files and directories
            continue;
        }
        if (type == FileType::ALL || currentType == type) {
            result.emplace_back(dp->d_name);
        }
    }

    return result;
}

bool move(const std::string& source, const std::string& destination) {
    ACSDK_INFO(LX("move").d("source", source).d("destination", destination));
    auto result = std::rename(source.c_str(), destination.c_str());
    if (result != 0) {
        ACSDK_ERROR(LX("move").m("Move failed").d("error", getStrError(errno)));
        return false;
    }

    return true;
}

std::string basenameOf(const std::string& path) {
    static std::mutex basenameMutex;
    auto lastSlash = path.find_last_not_of('/') + 1;

    if (lastSlash == 0) {
        return "";
    }

    std::unique_ptr<char[]> pathPtr(new char[path.size() + 1]{});
    std::copy(path.begin(), path.begin() + static_cast<ssize_t>(lastSlash), pathPtr.get());

    std::lock_guard<std::mutex> lock(basenameMutex);
    return basename(pathPtr.get());  // NOLINT(concurrency-mt-unsafe) protected by static mutex
}

std::string parentDirNameOf(const std::string& path) {
    static std::mutex dirnameMutex;
    auto lastSlash = path.find_last_not_of('/') + 1;

    if (lastSlash == 0 && path[0] == '/') {
        return "/";
    }

    std::unique_ptr<char[]> pathPtr(new char[path.size() + 1]{});
    std::copy(path.begin(), path.end(), pathPtr.get());

    std::lock_guard<std::mutex> lock(dirnameMutex);
    std::string result = dirname(pathPtr.get());  // NOLINT(concurrency-mt-unsafe) protected by static mutex
    if (result == "/") {
        return result;
    }
    return result + "/";
}

bool removeAll(const std::string& path) {
    ACSDK_INFO(LX("removeAll").d("path", path));
    struct stat pathStat {};
    if (lstat(path.c_str(), &pathStat) != 0) {
        ACSDK_DEBUG7(LX("removeAll").m("Path does not exists").d("path", path));
        return true;
    }

    if (!S_ISDIR(pathStat.st_mode)) {
        if (std::remove(path.c_str()) != 0) {
            ACSDK_ERROR(LX("removeAll").m("Failed to delete file").d("path", path));
            return false;
        }
        return true;
    }

    auto func = [](const char* fpath, const struct stat* s, int i, struct FTW* f) -> int {
        auto rv = std::remove(fpath);
        if (rv != 0) {
            ACSDK_ERROR(LX("removeAll").m("Failed to delete file").d("path", fpath));
        }

        return rv;
    };

    static constexpr auto maxOpenFd = 64;
    static std::mutex nftwMutex;
    std::lock_guard<std::mutex> lock(nftwMutex);
    return 0 == nftw(path.c_str(), func, maxOpenFd, FTW_DEPTH | FTW_PHYS);  // NOLINT(concurrency-mt-unsafe) protected
}

static size_t sizeOfDirectory(const std::string& rootDirectory) {
    static std::mutex sizeOfMutex;
    std::lock_guard<std::mutex> lock(sizeOfMutex);
    std::stack<std::string> directories;
    size_t result = 0;
    directories.push(rootDirectory);

    do {
        auto directory = std::move(directories.top());
        directories.pop();
        auto dirp = std::unique_ptr<DIR, int (*)(DIR*)>(opendir(directory.c_str()), &closedir);
        while (dirp != nullptr) {
            auto dp = readdir(dirp.get());  // NOLINT(concurrency-mt-unsafe) protected by static mutex
            if (dp == nullptr) {
                break;
            }
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                continue;
            }

            auto subPath = directory + "/" + dp->d_name;
            struct stat s {};
            if (lstat(subPath.c_str(), &s) != 0) {
                ACSDK_ERROR(LX("sizeOfDirectory").m("Subpath does not exists").d("path", subPath));
                continue;
            }

            if (S_ISDIR(s.st_mode)) {
                directories.emplace(std::move(subPath));
            } else {
                result += s.st_size;
            }
        }
    } while (!directories.empty());

    return result;
}

size_t sizeOf(const std::string& path) {
    struct stat pathStat {};
    if (lstat(path.c_str(), &pathStat) != 0) {
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
    struct statvfs diskStat {};
    if (statvfs(path.c_str(), &diskStat) != 0) {
        ACSDK_ERROR(LX("availableSpace").m("Failed to get free space from system").d("path", path));
        return 0;
    }
    return diskStat.f_bsize * diskStat.f_bavail;
}

bool pathContainsPrefix(const std::string& path, const std::string& prefix) {
    char resolved_path[PATH_MAX + 1];

    if (::realpath(path.c_str(), resolved_path) == nullptr) {
        auto errno_returned = errno;
        // check that it's not a file/directory doesn't exist error, which is acceptable
        if (errno_returned != ENOENT && errno_returned != ENOTDIR) {
            ACSDK_ERROR(LX("pathContainsPrefix")
                            .m("Unable to resolve path")
                            .d("path", path)
                            .d("error", getStrError(errno_returned)));
            return false;
        }
    }

    // assert resolved path is a prefix
    return strncmp(prefix.c_str(), resolved_path, prefix.length()) == 0;
}

}  // namespace filesystem
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // FILE_SYSTEM_UTILS_ENABLED