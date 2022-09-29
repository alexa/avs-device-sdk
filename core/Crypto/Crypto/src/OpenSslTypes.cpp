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

#include <acsdk/Crypto/private/OpenSslTypes.h>

namespace std {

std::ostream& operator<<(std::ostream& o, alexaClientSDK::crypto::PaddingMode mode) {
    const char* type;
    switch (mode) {
        case alexaClientSDK::crypto::PaddingMode::NoPadding:
            type = "NoPadding";
            break;
        case alexaClientSDK::crypto::PaddingMode::Padding:
            type = "Padding";
            break;
        default:
            return o << static_cast<int>(mode);
    }
    return o << type;
}

}  // namespace std
