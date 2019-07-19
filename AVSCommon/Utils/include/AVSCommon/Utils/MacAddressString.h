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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MACADDRESSSTRING_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MACADDRESSSTRING_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * A class used to validate a MAC address string before construction.
 */
class MacAddressString {
public:
    /// Default copy-constructor so objects can be passed by value.
    MacAddressString(const MacAddressString&) = default;

    /**
     * Factory that validates the MAC address before constructing the actual object.
     *
     * @params macAddress user supplied MacAddress
     * @return nullptr if the input MAC address is illegal, otherwise a unique_ptr to a MacAddressString object that can
     * be used to get the desired string.
     */
    static std::unique_ptr<MacAddressString> create(const std::string& macAddress);

    std::string getString() const;

private:
    /// The constructor will only be called with a legal macAddress input.  We don't check here because this function is
    /// private and is only called from the public create(...) factory method.
    MacAddressString(const std::string& macAddress);

    /// a well formed MAC address string
    const std::string m_macAddress;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_MACADDRESSSTRING_H_
