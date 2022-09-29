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

#include <acsdk/CryptoInterfaces/AlgorithmType.h>

namespace alexaClientSDK {
namespace cryptoInterfaces {

std::ostream& operator<<(std::ostream& out, AlgorithmType value) {
    const char* type;
    switch (value) {
        case AlgorithmType::AES_128_GCM:
            type = "AES-128-GCM";
            break;
        case AlgorithmType::AES_128_CBC:
            type = "AES-128-CBC";
            break;
        case AlgorithmType::AES_128_CBC_PAD:
            type = "AES-128-CBC-PAD";
            break;
        case AlgorithmType::AES_256_GCM:
            type = "AES-256-GCM";
            break;
        case AlgorithmType::AES_256_CBC:
            type = "AES-256-CBC";
            break;
        case AlgorithmType::AES_256_CBC_PAD:
            type = "AES-256-CBC-PAD";
            break;
        default:
            return out << static_cast<int>(value);
    }
    return out << type;
}

}  // namespace cryptoInterfaces
}  // namespace alexaClientSDK
