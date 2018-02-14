/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_FILE_FILEUTILS_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_FILE_FILEUTILS_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace file {

/**
 * A small utility function to help determine if a file exists.
 *
 * @param filePath The path to the file being queried about.
 * @return Whether the file exists and is accessible.
 */
bool fileExists(const std::string& filePath);

/**
 * A utility function to delete a file.
 *
 * @param filePath The path to the file being deleted.
 * @return Whether the file was deleted ok.
 */
bool removeFile(const std::string& filePath);

}  // namespace file
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_FILE_FILEUTILS_H_
