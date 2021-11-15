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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_FILESYSTEM_FILESYSTEMUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_FILESYSTEM_FILESYSTEMUTILS_H_

#include <string>
#include <vector>

/**
 * This utility file adds a few helper functions to interact with the file system.
 *
 * This synchronizes the interactions with the file system across different operating systems.
 * A reference implementation will be provided for the following systems by default:
 *   Linux, Mac OS, Raspberry Pi, Android, and Windows (UWP & Win32)
 *
 * If a platform is not supported and a custom implementation is not provided, then FILE_SYSTEM_UTILS_ENABLED will not
 * be defined and these utilities will not be available or compiled.
 *
 * @note Thread Safety: These utility provide a certain level of thread safety when it comes to using the specific APIs.
 * For example, traversing a directory with list will synchronize the various calls from within the process.
 * However, the effect of these utilities on the filesystem is not synchronized, and performing various unsynchronized
 * operations on the same set of directories can lead to unpredictable behavior.
 * For example, deleting a directory while listing it's content or returning its size.
 *
 * @note Permissions: Some systems do not differentiate between owner/group/other. For those systems (ie. Windows),
 * setting any attribute (read/write/exec) will set it for all. Some systems may also ignore some of these permissions.
 *
 * @note Case-Sensitivity: Some OS have case-insensitive filesystem (such as Windows by default) while others have
 * case-sensitive filesystem (such as Linux based OS). These utilities do not make a hard distinction between those and
 * the expectation for search and file checks will be dependant on the OS.
 *
 * @note Path Delimiters: Some use '/' as a directory delimiter, while others use '\'. This utility will always accept
 * '/' as a valid delimiter.
 * For OS that use '\' (such as Windows) then '\' will also be accepted.
 * For functions that return a path, then the path returned will use the default delimiter used by the OS.
 */
namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace filesystem {

/// Permission mask used to set the permissions of a file or directory
using Permissions = uint32_t;

/**
 * Permission bits used to distinguish the various permissions which can be set for a file or directory.
 * If a platform does not differentiate between owner/group/other, then setting read/write for any will set for all.
 */
enum PermBits : Permissions {
    NO_PERM = 0000,

    // Owner permissions
    OWNER_ALL = 0700,
    OWNER_READ = 0400,
    OWNER_WRITE = 0200,
    OWNER_EXEC = 0100,

    // Group permissions
    GROUP_ALL = 0070,
    GROUP_READ = 0040,
    GROUP_WRITE = 0020,
    GROUP_EXEC = 0010,

    // Everyone else's permissions
    OTHERS_ALL = 0007,
    OTHERS_READ = 0004,
    OTHERS_WRITE = 0002,
    OTHERS_EXEC = 0001
};

/** Default Permissions for directories, Read/Write/Exec for user, Read/Exec for group, none for everyone else. */
extern const Permissions DEFAULT_DIRECTORY_PERMISSIONS;
/** Default Permissions for files, Read/Write for user, Read for group, none for everyone else. */
extern const Permissions DEFAULT_FILE_PERMISSIONS;

/**
 * Enum for distinguishing different file types to drive various operations.
 *
 * REGULAR_FILE: Normal file types (not symlinks, block, character, fifo, or socket files)
 * DIRECTORY: Directory file type (does not include symbolic to directories).
 * ALL: All of the above supported types.
 *
 * @note hidden files or .files are considered Regular Files.
 */
enum class FileType { REGULAR_FILE, DIRECTORY, ALL };

/**
 * Changes the permissions of a given file or directory.
 * @note the outcome of this operation depends on the OS, see Permissions note above.
 *
 * @param path relative (to currentDirectory) or absolute path to a file or directory.
 * @param perms new permissions to override the existing ones.
 * @return true if the operation was successful, false otherwise.
 */
bool changePermissions(const std::string& path, Permissions perms);

/**
 * Checks if a file or directory already exists.
 *
 * @param path absolute or relative (to currentDirectory) path to a file or directory.
 * @return true if the file or directory exists, false otherwise.
 *
 * @note If you do not have permissions for this path, then this will return false even if it exists.
 */
bool exists(const std::string& path);

/**
 * Retrieves the current directory, usually the directory from which the program was started.
 *
 * @return current directory path.
 *
 * @note the delimiter used for the returned path will be dependant on the OS default path delimiter.
 */
std::string currentDirectory();

/**
 * Makes a directory on the given path with the provided permissions.
 *
 * @param path absolute path to a directory to be created.
 * @param perms Optional permissions to set the directory when created (or change them if they already exists).
 * @return true if the directory was successfully created or already exists.
 *
 * @note This will create all the necessary parent directories up until the provided path if they do not exist.
 * @note Any directory created recursively by this method will be set to the given permissions. If the final directory
 * already exists, then this method will attempt to set the permission of the given directory according to the given
 * param, and will return true only if the change permission operation succeeds.
 */
bool makeDirectory(const std::string& path, Permissions perms = DEFAULT_DIRECTORY_PERMISSIONS);

/**
 * Returns a list of directories, or files, or both in a given path.
 *
 * @param path absolute or relative (to currentDirectory) path to a given directory.
 * @param type Optional file type to list, defaults to all files and directories.
 * @return a standard vector of the desired file types found in the given path.
 *
 * @note This does not include ".", "..", any links, fifo, socket, or other special files that are not explicitly
 * called out as supported file types by FileType enum (see enum description).
 * @note Changing the content of the given directory in the middle of this call can lead to unknown behavior.
 */
std::vector<std::string> list(const std::string& path, FileType type = FileType::ALL);

/**
 * Moves/renames a source path (file or directory) to a new destination.
 *
 * @param source existing path to be moved or renamed.
 * @param destination desired destination to move to, the parent path of the destination must exist.
 * @return true if the move was successful, false otherwise.
 */
bool move(const std::string& source, const std::string& destination);

/**
 * Returns the basename from a given path (regardless if it exists or not).
 *
 * "/some/file.txt" --> "file"
 * "/some/dir/"     --> "dir"
 * "/some/dir/.."   --> ".."
 * "."              --> "."
 * "/"              --> ""
 * ""               --> ""
 *
 * @param path relative (to currentDirectory) or absolute path to a file or directory.
 * @return basename of the given path.
 */
std::string basenameOf(const std::string& path);

/**
 * Returns the directory name of a given path with a trailing '/' (regardless if it exists or not).
 * @note if an OS has a concept of drives (ie. Windows) then it will return the drive letter if provided in the path.
 *
 * "/some/file.txt" --> "/some/"
 * "/some/dir/"     --> "/some/"
 * "/some/dir/.."   --> "/some/dir/"
 * "."              --> "./"
 * "/"              --> "/"
 * ""               --> "./"
 * "C:/path/file"   --> "C:/path"
 *
 * @param path relative (to currentDirectory) or absolute path to a file or directory.
 * @return dirname of the given path.
 *
 * @note the delimiter used for the returned path will be dependant on the OS default path delimiter.
 */
std::string parentDirNameOf(const std::string& path);

/**
 * Removes a given file or recursively removes all files in a given directory.
 *
 * @param path file or directory to remove recursively.
 * @return true if the path was removed completely or did not exist in the first place, false otherwise.
 *
 * @note This does not follow any symbolic links to directories outside the given path. Only the links will be removed.
 */
bool removeAll(const std::string& path);

/**
 * Get the size in bytes of a given file or directory recursively. If a directory is given, then this will return the
 * sum of all the file sizes recursively under the given directory, without counting the size of each directory.
 * This function does not follow any symbolic links, but simply return the size of the link itself.
 *
 * @param path file or directory for which to calculate the size.
 * @return size of the given path in bytes.
 *
 * @note Changing the content of the given directory in the middle of this call can lead to unknown behavior.
 */
size_t sizeOf(const std::string& path);

/**
 * Get the number of bytes available for writing on a given path.
 *
 * @param path of the directory that will be written to (directory must exist).
 * @return number of bytes available to write.
 *
 * @note The result will only provide the size available at the time of calling this function, caller is responsible
 * for synchronizing space allocation while utilizing this function to avoid running out of space.
 */
size_t availableSpace(const std::string& path);

/**
 * Assert that the given path starts with the prefix after resolving any file traversal tokens or symlinks.
 *
 * @param path of the directory that will be checked.
 * @param prefix the expected prefix of the path after resolution.
 * @return true if the path starts with prefix.
 */
bool pathContainsPrefix(const std::string& path, const std::string& prefix);

}  // namespace filesystem
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_FILESYSTEM_FILESYSTEMUTILS_H_

#endif  // FILE_SYSTEM_UTILS_ENABLED
