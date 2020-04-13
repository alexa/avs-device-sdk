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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_UUIDGENERATION_UUIDGENERATION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_UUIDGENERATION_UUIDGENERATION_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace uuidGeneration {

/**
 * Generates a variant 1, version 4 universally unique identifier (UUID) consisting of 32 hexadecimal digits.
 * The UUID generated is of the format xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx where M indicates the version, and the two
 * most significant bits of N indicates the variant. M is 0100 (binary) for version 4 and N is 10xx(binary) for
 * variant 1.
 * @see https://tools.ietf.org/html/rfc4122.
 *
 * @return A uuid as a string.
 */
const std::string generateUUID();

/**
 * Allows caller to set a specific salt to be used in any seeding operation.
 * Salt wil be a prefix to the seed and should be as specific to the unique device as possible.
 * Setting a salt will cause the next UUID to be generated with a new seed.
 *
 * @param newSalt  the salt to use
 */
void setSalt(const std::string& newSalt);

}  // namespace uuidGeneration
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_UUIDGENERATION_UUIDGENERATION_H_
