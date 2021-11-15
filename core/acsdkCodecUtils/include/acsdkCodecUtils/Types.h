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

#ifndef ACSDKCODECUTILS_TYPES_H_
#define ACSDKCODECUTILS_TYPES_H_

#include <vector>

namespace alexaClientSDK {
namespace acsdkCodecUtils {

/// @brief Byte data type.
/// @ingroup CodecUtils
typedef unsigned char Byte;

/// @brief Byte data block.
/// @ingroup CodecUtils
typedef std::vector<Byte> Bytes;

}  // namespace acsdkCodecUtils
}  // namespace alexaClientSDK

#endif  // ACSDKCODECUTILS_TYPES_H_
