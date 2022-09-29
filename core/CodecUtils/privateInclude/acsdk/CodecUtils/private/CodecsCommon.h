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

#ifndef ACSDK_CODECUTILS_PRIVATE_CODECSCOMMON_H_
#define ACSDK_CODECUTILS_PRIVATE_CODECSCOMMON_H_

namespace alexaClientSDK {
namespace codecUtils {

/**
 * @brief Method to determine is character is ignorable when decoding data.
 *
 * This method tells if the character shall be ignored when converting strings into binary.
 *
 * @param [in] ch Character position to test.
 *
 * @retval true If \a ch character is ignored when decoding string.
 * @retval false If \a ch character is not ignorable whitespace.
 * @private
 */
bool isIgnorableWhitespace(char ch);

}  // namespace codecUtils
}  // namespace alexaClientSDK

#endif  // ACSDK_CODECUTILS_PRIVATE_CODECSCOMMON_H_
