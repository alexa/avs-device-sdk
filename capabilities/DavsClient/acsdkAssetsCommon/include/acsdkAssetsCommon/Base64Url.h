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

#ifndef ACSDKASSETSCOMMON_BASE64URL_H_
#define ACSDKASSETSCOMMON_BASE64URL_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

/**
 * This is added to wrap Base64 functionality because we have no clean way of using it across all cases.
 * We need to use Base64 on device and we'll use libssl (BoringSSL) for that, but the code in OpenSSL and BoringSSL
 * differs for Base64! :( So for unit tests, and for running on Mac/Ubuntu, we'll use code copied over from mbedtls.
 */
class Base64Url {
public:
    /**
     * Encodes plain text into URL-friendly Base64 version.
     * @param plain IN The text to encode.
     * @param encoded OUT Base64 string but further modified so that '+', '/' and '=' are converted to '%2B', '%2F' and
     * '%3D' respectively.
     * @return true if successful
     */
    static bool encode(const std::string& plain, std::string& encoded);

    // Reverse operation from encode().
    // NOTE: actually, this is not used in production, it is only used by unit tests.
    static bool decode(const std::string& encoded, std::string& plain);

private:
    Base64Url() = delete;
    ~Base64Url() = delete;
};

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETSCOMMON_BASE64URL_H_
