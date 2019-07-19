/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_COMMON_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_COMMON_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Utility function to generate a random string of characters between 'a' - 'z'.
 *
 * @param stringSize The length of the string we will create.
 * @return The generated string.
 */
std::string createRandomAlphabetString(int stringSize);

/**
 * Utility function to generate a random number.  This function does not require seeding before use.
 *
 * @param min The minimum value in the desired distribution.
 * @param max The maximum value in the desired distribution.
 * @return The generated number.
 */
int generateRandomNumber(int min, int max);

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_TEST_AVSCOMMON_UTILS_COMMON_COMMON_H_
