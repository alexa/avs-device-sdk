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

#include <AVSCommon/Utils/MacAddressString.h>

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

std::unique_ptr<MacAddressString> MacAddressString::create(const std::string& macAddress) {
    enum class State { FIRST_OCTET_BYTE, SECOND_OCTET_BYTE, DIVIDER };

    State parseState = State::FIRST_OCTET_BYTE;
    int numOctets = 0;
    for (const auto& c : macAddress) {
        switch (parseState) {
            case State::FIRST_OCTET_BYTE:
                if (!std::isxdigit(c)) {
                    return nullptr;
                }
                parseState = State::SECOND_OCTET_BYTE;
                break;
            case State::SECOND_OCTET_BYTE:
                if (!std::isxdigit(c)) {
                    return nullptr;
                }
                parseState = State::DIVIDER;
                ++numOctets;
                break;
            case State::DIVIDER:
                if ((':' != c) && ('-' != c)) {
                    return nullptr;
                }
                parseState = State::FIRST_OCTET_BYTE;
                break;
        }
    }

    if ((6 != numOctets) || (parseState != State::DIVIDER)) {
        return nullptr;
    }

    return std::unique_ptr<MacAddressString>(new MacAddressString(macAddress));
}

std::string MacAddressString::getString() const {
    return m_macAddress;
}

MacAddressString::MacAddressString(const std::string& macAddress) : m_macAddress(macAddress) {
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
