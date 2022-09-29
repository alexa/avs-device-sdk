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

#ifndef ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYER_INCLUDE_ACSDKAUDIOPLAYER_UTIL_H_
#define ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYER_INCLUDE_ACSDKAUDIOPLAYER_UTIL_H_

#include <string>
#include <acsdk/CryptoInterfaces/CryptoFactoryInterface.h>

namespace alexaClientSDK {
namespace acsdkAudioPlayer {

class Util {
public:
    /**
     * Generate MD5 Hash.
     *
     * @param input to hash.
     * @param cryptoFactory Encryption facilities factory.
     * @return string MD5 Hash of input or an empty string if hashing fails.
     */
    static std::string generateMD5Hash(
        const std::string& input,
        const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory);

    /**
     * Get domain name from an Url "https://www.xyz.com/sfsf" will return
     * "www.xyz.com".
     *
     * @param url to parse
     * @return string of the hostname found in Url. Empty string if parsing is unsuccessful.
     */
    static std::string getDomainNameFromUrl(const std::string& url);
};

}  // namespace acsdkAudioPlayer
}  // namespace alexaClientSDK

#endif  // #define ALEXA_CLIENT_SDK_ACSDKAUDIOPLAYER_INCLUDE_ACSDKAUDIOPLAYER_UTIL_H_
